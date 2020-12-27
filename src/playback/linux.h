#pragma once
#ifdef __linux__
#include <array>
#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "global.h"
#include "../config/config.h"

namespace Soundux
{
    namespace Playback
    {
        namespace internal
        {
            inline std::string sinkId;
            inline const std::string sinkName = "soundboard_sink";

            inline std::string getOutput(const std::string &command)
            {
                std::array<char, 128> buffer;
                std::string result;

                std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

                if (!pipe)
                {
                    throw std::runtime_error("popen failed");
                }
                while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
                {
                    result += buffer.data();
                }
                return result;
            }

            inline std::vector<std::string> splitByNewLine(const std::string &str)
            {
                auto result = std::vector<std::string>{};
                auto ss = std::stringstream{str};

                for (std::string line; std::getline(ss, line, '\n');)
                {
                    result.push_back(line);
                }

                return result;
            };

            inline std::string getDefaultCaptureDevice()
            {
                // get default input device
                std::string defaultInput;
                char cmd[] = "LC_ALL=C pactl info";
                auto result = getOutput(cmd);
                std::regex reg(R"rgx(Default Source: (.+))rgx");
                std::smatch sm;
                regex_search(result, sm, reg);
                defaultInput = sm[1].str();
                return defaultInput;
            }

            struct PulseAudioRecordingStream
            {
                int index = -1;
                std::string driver;
                std::string source;
                std::string resampleMethod;
                std::string processBinary;

                operator bool() const
                {
                    return index >= 0;
                }
            };

            inline bool isValidDevice(const PulseAudioRecordingStream &stream)
            {
                return stream.driver == "protocol-native.c" && stream.resampleMethod != "peaks";
            }
        } // namespace internal

        inline void createSink()
        {
            system(("pactl load-module module-null-sink sink_name=" + internal::sinkName +
                    " rate=44100 sink_properties=device.description=" + internal::sinkName + " > /dev/null")
                       .c_str());

            auto defaultInput = internal::getDefaultCaptureDevice();
            // Create loopback for input
            if (!defaultInput.empty())
            {
                auto createLoopBack = "pactl load-module module-loopback rate=44100 source=\"" + defaultInput +
                                      "\" sink=\"" + internal::sinkName + "\" > /dev/null";

                static_cast<void>(system(createLoopBack.c_str()));
            }

            auto sources = internal::getOutput("LC_ALL=C pactl list sources");
            auto sourcesSplit = internal::splitByNewLine(sources);

            struct
            {
                std::string id;
                std::string name;
            } device{};

            static const std::regex sourceRegex(R"rgx((.*#(\d+))$|(Name: (.+)))rgx");
            std::smatch match;

            for (const std::string &line : sourcesSplit)
            {
                if (std::regex_search(line, match, sourceRegex))
                {
                    if (match[2].matched)
                    {
                        device.id = match[2];
                    }
                    else if (match[4].matched)
                    {
                        device.name = match[4];
                    }

                    if (device.name == internal::sinkName + ".monitor")
                    {
                        break;
                    }
                }
            }

            if (device.name != internal::sinkName + ".monitor")
            {
                std::cerr << "Failed to find soundboard sink in PulseAudio sources!" << std::endl;
            }

            internal::sinkId = device.id;
        };
        inline void deleteSink()
        {
            // TODO(d3s0x): only unload soundboard sink
            system("pactl unload-module module-null-sink 2> /dev/null");
            system("pactl unload-module module-loopback 2> /dev/null");
        };
        inline auto getSources()
        {
            using internal::getOutput;
            using internal::PulseAudioRecordingStream;
            using internal::splitByNewLine;

            auto input = getOutput("LC_ALL=C pactl list source-outputs");

            auto splitted = splitByNewLine(input);
            std::vector<PulseAudioRecordingStream> streams;

            static auto regex = std::regex(
                R"rgx((.*#(\d+))|(Driver: (.+))|(Source: (\d+))|(.*process.*binary.* = "(.+)")|(Resample method: (.+)))rgx");

            PulseAudioRecordingStream stream;
            for (auto &line : splitted)
            {
                std::smatch match;
                if (std::regex_search(line, match, regex))
                {
                    if (match[2].matched)
                    {
                        if (stream && isValidDevice(stream))
                        {
                            streams.push_back(stream);
                        }

                        stream = {};
                        stream.index = std::stoi(match[2]);
                    }
                    else if (stream)
                    {
                        if (match[4].matched)
                        {
                            stream.driver = match[4];
                        }
                        else if (match[6].matched)
                        {
                            stream.source = match[6];
                        }
                        else if (match[8].matched)
                        {
                            stream.processBinary = match[8];
                        }
                        else if (match[10].matched)
                        {
                            stream.resampleMethod = match[10];
                        }
                    }
                }
            }

            if (stream && isValidDevice(stream))
            {
                streams.push_back(stream);
            }

            return streams;
        }
        inline std::optional<internal::PulseAudioRecordingStream> getCurrentOutputApplication()
        {
            auto sources = Soundux::Playback::getSources();
            if (sources.size() > Soundux::Config::gConfig.currentOutputApplication)
            {
                return sources[Soundux::Config::gConfig.currentOutputApplication];
            }
            return std::nullopt;
        }
    } // namespace Playback
} // namespace Soundux
#endif