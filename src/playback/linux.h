#pragma once
#ifdef __linux__
#include <stdexcept>
#include <exception>
#include <optional>
#include <iostream>
#include <stdio.h>
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
                std::string defaultInput = "";
                char cmd[] = "pacmd dump";
                auto result = getOutput(cmd);
                std::regex reg(R"rgx(set-default-source (.+))rgx");
                std::smatch sm;
                regex_search(result, sm, reg);
                defaultInput = sm[1].str();
                return defaultInput;
            }

            struct PulseAudioRecordingStream
            {
                int index = -1;
                std::string driver;
                std::string flags;
                std::string state;
                std::string source;
                bool muted;
                std::string applicationName;
                int processId;
                std::string processBinary;

                operator bool()
                {
                    return index >= 0;
                }
            };

            inline bool isValidDevice(const PulseAudioRecordingStream &stream)
            {
                return (stream.source == internal::sinkName + ".monitor" ||
                        stream.source.find(".monitor") == std::string::npos) &&
                       stream.flags.find("DONT_MOVE") == std::string::npos && stream.driver == "<protocol-native.c>";
            }
        } // namespace internal

        inline std::string createSink()
        {
            auto sink = internal::getOutput("pacmd load-module module-null-sink sink_name=" + internal::sinkName +
                                            " sink_properties=device.description=" + internal::sinkName);

            auto defaultInput = internal::getDefaultCaptureDevice();
            // Create loopback for input
            if (defaultInput != "")
            {
                auto createLoopBack = "pacmd load-module module-loopback source=\"" + defaultInput + "\" sink=\"" +
                                      internal::sinkName + "\"";
                system(createLoopBack.c_str());
            }
            return internal::sinkName;
        };
        inline void deleteSink()
        {
            auto sink = internal::getOutput("pacmd unload-module module-null-sink");
            auto loopback = internal::getOutput("pacmd unload-module module-loopback");
        };
        inline auto getSources()
        {
            using namespace internal;

            auto input = getOutput("pacmd list-source-outputs");

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
                R"rgx(((index: (\d+)))|(driver: )(.*)|(state: )(.*)|(flags: )(.*)|(source: .*)(<(.*)>)|(muted: )(.{0,3})|([a-zA-Z-.0-9_]*)\ =\ (\"(.*)\"))rgx");

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
                        stream.index = std::stoi(match[3]);
                    }
                    else if (stream)
                    {
                        if (match[4].matched)
                            stream.driver = match[5];
                        else if (match[8].matched)
                            stream.flags = match[9];
                        else if (match[6].matched)
                            stream.state = match[7];
                        else if (match[10].matched)
                            stream.source = match[12];
                        else if (match[13].matched)
                            stream.muted = match[14] == "yes" ? true : false;
                        else if (match[15].matched)
                        {
                            auto currentProperty = match[15];
                            auto currentValue = match[17];

                            if (currentProperty == "application.name")
                            {
                                stream.applicationName = currentValue;
                            }
                            else if (currentProperty == "application.process.id")
                            {
                                stream.processId = std::stoi(currentValue);
                            }
                            else if (currentProperty == "application.process.binary")
                            {
                                stream.processBinary = currentValue;
                            }
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