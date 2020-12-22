#pragma once
#ifdef __linux__
#include <stdexcept>
#include <exception>
#include <optional>
#include <iostream>
#include <cstdio>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <regex>

#include "global.h"
#include "../config/config.h"

namespace Soundux
{
    namespace Playback
    {
        namespace internal
        {
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

            inline std::string getDefaultCaptureDevice()
            {
                // get default input device
                std::string defaultInput;
                char cmd[] = "pactl info";
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

                operator bool()
                {
                    return index >= 0;
                }
            };

            inline bool isValidDevice(const PulseAudioRecordingStream &stream)
            {
                return stream.driver == "protocol-native.c" && stream.resampleMethod != "peaks";
            }
        } // namespace internal

        inline std::string createSink()
        {
            system(("pactl load-module module-null-sink sink_name=" + internal::sinkName +
                                            " sink_properties=device.description=" + internal::sinkName + " > nul").c_str());

            auto defaultInput = internal::getDefaultCaptureDevice();
            // Create loopback for input
            if (!defaultInput.empty())
            {
                auto createLoopBack = "pactl load-module module-loopback source=\"" + defaultInput + "\" sink=\"" +
                                      internal::sinkName + "\" > nul";

                static_cast<void>(system(createLoopBack.c_str()));
            }
            return internal::sinkName;
        };
        inline void deleteSink()
        {
            // TODO: only unload soundboard sink
            system("pactl unload-module module-null-sink 2> nul");
            system("pactl unload-module module-loopback 2> nul");
        };
        inline auto getSources()
        {
            using namespace internal;

            auto input = getOutput("pactl list source-outputs");

            static auto splitByNewLine = [](const std::string &str) {
                auto result = std::vector<std::string>{};
                auto ss = std::stringstream{str};

                for (std::string line; std::getline(ss, line, '\n');)
                    result.push_back(line);

                return result;
            };

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
                            stream.driver = match[4];
                        else if (match[6].matched)
                            stream.source = match[6];
                        else if (match[8].matched)
                            stream.processBinary = match[8];
                        else if (match[10].matched)
                            stream.resampleMethod = match[10];
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