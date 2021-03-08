#if defined(__linux__)
#include "pulse.hpp"
#include <fancy.hpp>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>

bool exec(const std::string &command, std::string &result)
{
    result.clear();

    std::array<char, 128> buffer;
    auto *pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen failed");
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }

    return pclose(pipe) == 0;
}
auto splitByNewLine(const std::string &str)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    for (std::string line; std::getline(ss, line, '\n');)
    {
        result.emplace_back(line);
    }
    return result;
}

namespace Soundux::Objects
{
    bool Pulse::setModuleId(const std::string &command, std::uint32_t &id)
    {
        static const std::regex idRegex(R"((\d+))");
        std::string output;
        std::smatch match;

        if (exec(command, output))
        {
            if (std::regex_search(output, match, idRegex))
            {
                id = std::stoi(match[1]);
            }
            else
            {
                Fancy::fancy.logTime().failure() << "Command: " >> command << " returned unexpected result"
                                                                           << std::endl;
                unloadLeftOverModules();
                return false;
            }
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Command: " >> command << " failed" << std::endl;
            unloadLeftOverModules();
            return false;
        }

        return true;
    }
    void Pulse::setup()
    {
        unloadLeftOverModules();
        refreshPlaybackStreams();
        refreshRecordingStreams();
        fetchDefaultPulseSource();

        if (!setModuleId("pactl load-module module-null-sink sink_name=soundux_sink rate=44100 "
                         "sink_properties=device.description=soundux_sink",
                         data.nullSinkModuleId))
        {
            return;
        }
        if (!setModuleId("pactl load-module module-loopback rate=44100 source=" + data.pulseDefaultSource +
                             " sink=soundux_sink sink_dont_move=true",
                         data.loopbackModuleId))
        {
            return;
        }
        if (!setModuleId("pactl load-module module-null-sink sink_name=soundux_sink_passthrough rate=44100 "
                         "sink_properties=device.description=soundux_sink_passthrough",
                         data.passthroughModuleId))
        {
            return;
        }
        if (!setModuleId("pactl load-module module-loopback latency_msec=1 "
                         "source=soundux_sink_passthrough.monitor sink=soundux_sink",
                         data.passthroughLoopbackSinkModuleId))
        {
            return;
        }
        if (!setModuleId("pactl load-module module-loopback source=soundux_sink_passthrough.monitor",
                         data.passthroughLoopbackMonitorModuleId))
        {
            return;
        }

        refreshPlaybackStreams(true);
        refreshRecordingStreams(true);

        static const std::regex sourceRegex(R"rgx((.*#(\d+))$|(Name: (.+)))rgx");
        std::string sources;
        if (exec("LC_ALL=C pactl list sources", sources))
        {
            std::smatch match;

            std::string deviceId;
            for (const auto &line : splitByNewLine(sources))
            {
                if (std::regex_search(line, match, sourceRegex))
                {
                    if (match[2].matched)
                    {
                        deviceId = match[2];
                    }

                    if (match[4].matched && match[4] == "soundux_sink.monitor")
                    {
                        data.sinkMonitorId = std::stoi(deviceId);
                        return;
                    }
                }
            }

            Fancy::fancy.logTime().failure() << "Failed to find monitor of soundux sink!" << std::endl;
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Failed to get sources" << std::endl;
        }
    }
    void Pulse::destroy()
    {
        moveBackCurrentApplication();
        moveBackApplicationFromPassthrough();
        revertDefaultSourceToOriginal();

        if (data.loopbackModuleId != 0)
        {
            if (system(("pactl unload-module " + std::to_string(data.loopbackModuleId)).c_str()) != 0) // NOLINT
            {
                Fancy::fancy.logTime().warning() << "Could not unload loopback module" << std::endl;
            }
        }
        if (data.nullSinkModuleId != 0)
        {
            // NOLINTNEXTLINE
            if (system(("pactl unload-module " + std::to_string(data.nullSinkModuleId)).c_str()) != 0)
            {
                Fancy::fancy.logTime().warning() << "Could not unload nullsink module" << std::endl;
            }
        }
        if (data.passthroughModuleId != 0)
        {
            // NOLINTNEXTLINE
            if (system(("pactl unload-module " + std::to_string(data.passthroughModuleId)).c_str()) != 0)
            {
                Fancy::fancy.logTime().warning() << "Could not unload passthrough module" << std::endl;
            }
        }
        if (data.passthroughLoopbackSinkModuleId != 0)
        {
            // NOLINTNEXTLINE
            if (system(("pactl unload-module " + std::to_string(data.passthroughLoopbackSinkModuleId)).c_str()) != 0)
            {
                Fancy::fancy.logTime().warning() << "Could not unload passtrhough loopback module" << std::endl;
            }
        }
        if (data.passthroughLoopbackMonitorModuleId != 0)
        {
            // NOLINTNEXTLINE
            if (system(("pactl unload-module " + std::to_string(data.passthroughLoopbackMonitorModuleId)).c_str()) != 0)
            {
                Fancy::fancy.logTime().warning() << "Could not unload passtrhough monitor module" << std::endl;
            }
        }
    }
    void Pulse::fetchDefaultPulseSource()
    {
        std::string info;
        if (exec("LC_ALL=C pactl info", info))
        {
            static const std::regex defaultDeviceRegex("^Default Source: (.+)|Server Name: (.*)$");

            std::smatch match;
            for (const auto &line : splitByNewLine(info))
            {
                if (std::regex_match(line, match, defaultDeviceRegex))
                {
                    if (match[1].matched)
                    {
                        if (match[1].str() == "soundux_sink.monitor")
                        {
                            Fancy::fancy.logTime().warning()
                                << "Default Source is Soundux Sink, this should not happen!" << std::endl;
                            return;
                        }
                        data.pulseDefaultSource = match[1];
                        Fancy::fancy.logTime()
                            << "Default Pulse Source was saved: " << data.pulseDefaultSource << std::endl;
                    }
                    if (match[2].matched)
                    {
                        if (match[2].str().find("PipeWire") != std::string::npos)
                        {
                            Fancy::fancy.logTime().warning() << "Detected PipeWire, it will probably take a lot of "
                                                                "time until the UI loads because loading " >>
                                "module-looback"
                                    << " will result in a timeout" << std::endl;
                        }
                    }
                }
            }
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Failed to get default pulse sources" << std::endl;
        }
    }
    bool Pulse::revertDefaultSourceToOriginal()
    {
        if (!data.pulseDefaultSource.empty())
        {
            if (system(("pactl unload-module " + std::to_string(data.loopbackModuleId)).c_str()) != 0) // NOLINT
            {
                Fancy::fancy.logTime().warning() << "Could not unload loopback module" << std::endl;
                return false;
            }

            if (!setModuleId("pactl load-module module-loopback rate=44100 source=" + data.pulseDefaultSource +
                                 " sink=soundux_sink sink_dont_move=true",
                             data.loopbackModuleId))
            {
                Fancy::fancy.logTime().failure() << "Failed to re-create moveable loopback" << std::endl;
                return false;
            }

            if (system(("pactl set-default-source " + data.pulseDefaultSource + " >/dev/null").c_str()) != 0) // NOLINT
            {
                return false;
            }
        }
        else
        {
            Fancy::fancy.logTime().failure()
                << "Failed to revert default source, default source was not set!" << std::endl;
            return false;
        }
        return true;
    }
    bool Pulse::setDefaultSourceToSoundboardSink()
    {
        if (system(("pactl unload-module " + std::to_string(data.loopbackModuleId)).c_str()) != 0) // NOLINT
        {
            Fancy::fancy.logTime().warning() << "Could not unload loopback module" << std::endl;
            return false;
        }
        if (!setModuleId("pactl load-module module-loopback rate=44100 source=" + data.pulseDefaultSource +
                             " sink=soundux_sink",
                         data.loopbackModuleId))
        {
            Fancy::fancy.logTime().failure() << "Failed to re-create moveable loopback" << std::endl;
            return false;
        }

        return system("pactl set-default-source soundux_sink.monitor >/dev/null") == 0; // NOLINT
    }
    bool Pulse::moveApplicationToSinkMonitor(const std::string &streamName)
    {
        if (!currentApplication || (currentApplication && currentApplication->name != streamName))
        {
            moveBackCurrentApplication();
            refreshRecordingStreams();

            std::shared_lock lock(recordingStreamMutex);
            if (recordingStreams.find(streamName) != recordingStreams.end())
            {
                auto &stream = recordingStreams.at(streamName);
                currentApplication = stream;
                // NOLINTNEXTLINE
                return system(("pactl move-source-output " + std::to_string(stream.id) +
                               " soundux_sink.monitor >/dev/null")
                                  .c_str()) == 0;
            }
            Fancy::fancy.logTime().failure()
                << "Failed to find PulseRecordingStream with name: " << streamName << std::endl;
            return false;
        }
        return true;
    }
    bool Pulse::moveBackCurrentApplication()
    {
        if (currentApplication)
        {
            // NOLINTNEXTLINE
            auto success = system(("pactl move-source-output " + std::to_string(currentApplication->id) + " " +
                                   currentApplication->source + " >/dev/null")
                                      .c_str()) == 0;
            currentApplication = std::nullopt;
            return success;
        }
        return true; //* Not having anything to moveback should count as a failure
    }
    void Pulse::refreshRecordingStreams(const bool &fix)
    {
        std::string sourceList;
        if (exec("LC_ALL=C pactl list source-outputs", sourceList))
        {

            std::vector<PulseRecordingStream> fetchedStreams;
            static const auto recordingStreamRegex = std::regex(
                R"rgx((.*#(\d+))|(Driver: (.+))|(Source: (\d+))|(.*process.*binary.* = "(.+)")|(Resample method: (.+)))rgx");

            PulseRecordingStream stream;
            std::smatch match;

            for (const auto &line : splitByNewLine(sourceList))
            {
                if (std::regex_search(line, match, recordingStreamRegex))
                {
                    if (match[2].matched)
                    {
                        if (stream)
                        {
                            recordingStreamMutex.lock_shared();
                            if (recordingStreams.find(stream.name) != recordingStreams.end())
                            {
                                if (fix && stream.source != recordingStreams.at(stream.name).source)
                                {
                                    stream.source = recordingStreams.at(stream.name).source;

                                    // NOLINTNEXTLINE
                                    if (system(("pactl move-source-output " + std::to_string(stream.id) + " " +
                                                stream.source)
                                                   .c_str()) == 0)
                                    {
                                        Fancy::fancy.logTime().success()
                                            << "Recovered " << stream.name
                                            << " from left over module to original source: " << stream.source
                                            << std::endl;
                                    }
                                }
                            }
                            recordingStreamMutex.unlock_shared();
                            fetchedStreams.emplace_back(stream);
                        }

                        stream = {};
                        stream.id = std::stoi(match[2]);
                    }
                    else if (match[4].matched)
                    {
                        stream.driver = match[4];
                    }
                    else if (match[6].matched)
                    {
                        stream.source = match[6];
                    }
                    else if (match[8].matched)
                    {
                        stream.name = match[8];
                    }
                    else if (match[10].matched)
                    {
                        stream.resampleMethod = match[10];
                    }
                }
            }
            recordingStreamMutex.lock_shared();
            if (stream)
            {
                if (recordingStreams.find(stream.name) != recordingStreams.end())
                {
                    stream.source = recordingStreams.at(stream.name).source;
                }
                fetchedStreams.emplace_back(stream);
            }
            recordingStreamMutex.unlock_shared();

            std::unique_lock lock(recordingStreamMutex);
            recordingStreams.clear();
            for (const auto &stream : fetchedStreams)
            {
                recordingStreams.insert({stream.name, stream});
            }
        }
        else
        {
            Fancy::fancy.logTime() << "Failed to get recording streams" << std::endl;
        }
    }
    void Pulse::refreshPlaybackStreams(const bool &fix)
    {
        std::string sourceList;
        if (exec("LC_ALL=C pactl list sink-inputs", sourceList))
        {

            std::vector<PulsePlaybackStream> fetchedStreams;
            static const auto playbackStreamRegex =
                std::regex(R"rgx((.*#(\d+))|(Driver: (.+))|(Sink: (\d+))|(.*application\.name.* = "(.+)"))rgx");

            PulsePlaybackStream stream;
            std::smatch match;

            for (const auto &line : splitByNewLine(sourceList))
            {
                if (std::regex_search(line, match, playbackStreamRegex))
                {
                    if (match[2].matched)
                    {
                        if (stream)
                        {
                            playbackStreamMutex.lock_shared();
                            if (playbackStreams.find(stream.name) != playbackStreams.end())
                            {
                                if (fix && stream.sink != playbackStreams.at(stream.name).sink)
                                {
                                    stream.sink = playbackStreams.at(stream.name).sink;

                                    // NOLINTNEXTLINE
                                    if (system(("pactl move-sink-input " + std::to_string(stream.id) + " " +
                                                stream.sink + " >/dev/null")
                                                   .c_str()) == 0)
                                    {
                                        Fancy::fancy.logTime().success()
                                            << "Recovered " + stream.name << " from left over sink to original sink "
                                            << stream.sink << std::endl;
                                    }
                                }
                            }
                            playbackStreamMutex.unlock_shared();
                            fetchedStreams.emplace_back(stream);
                        }

                        stream = {};
                        stream.id = std::stoi(match[2]);
                    }
                    else if (match[4].matched)
                    {
                        stream.driver = match[4];
                    }
                    else if (match[6].matched)
                    {
                        stream.sink = match[6];
                    }
                    else if (match[8].matched)
                    {
                        stream.name = match[8];
                    }
                }
            }
            playbackStreamMutex.lock_shared();
            if (stream)
            {
                if (playbackStreams.find(stream.name) != playbackStreams.end())
                {
                    stream.sink = playbackStreams.at(stream.name).sink;
                }
                fetchedStreams.emplace_back(stream);
            }
            playbackStreamMutex.unlock_shared();

            std::unique_lock lock(playbackStreamMutex);
            playbackStreams.clear();
            for (const auto &stream : fetchedStreams)
            {
                playbackStreams.insert({stream.name, stream});
            }
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Failed to get playback streams" << std::endl;
        }
    }
    void Pulse::unloadLeftOverModules()
    {
        std::string loadedModules;
        if (exec("LC_ALL=C pactl list modules", loadedModules))
        {
            static const std::regex moduleRegex(R"rgx((Module #(\d+))|(Argument: .*(soundux_sink).*))rgx");

            std::smatch match;
            std::string currentModuleId;
            for (const auto &line : splitByNewLine(loadedModules))
            {
                if (std::regex_search(line, match, moduleRegex))
                {
                    if (match[2].matched)
                    {
                        currentModuleId = match[2];
                    }
                    else if (match[4].matched)
                    {
                        if (system(("pactl unload-module " + currentModuleId).c_str()) == 0) // NOLINT
                        {
                            Fancy::fancy.logTime().success()
                                << "Unloaded left over module " << currentModuleId << std::endl;
                        }
                    }
                }
            }
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Failed to unload left over modules" << std::endl;
        }
    }
    std::vector<PulseRecordingStream> Pulse::getRecordingStreams()
    {
        std::shared_lock lock(recordingStreamMutex);
        std::vector<PulseRecordingStream> rtn;
        for (const auto &stream : recordingStreams)
        {
            rtn.emplace_back(stream.second);
        }
        return rtn;
    }
    std::vector<PulsePlaybackStream> Pulse::getPlaybackStreams()
    {
        std::shared_lock lock(playbackStreamMutex);
        std::vector<PulsePlaybackStream> rtn;
        for (const auto &stream : playbackStreams)
        {
            rtn.emplace_back(stream.second);
        }
        return rtn;
    }
    bool Pulse::moveBackApplicationFromPassthrough()
    {
        if (currentApplicationPassthrough)
        {
            // NOLINTNEXTLINE
            return system(("pactl move-sink-input " + std::to_string(currentApplicationPassthrough->id) + " " +
                           currentApplicationPassthrough->sink + " >/dev/null")
                              .c_str()) == 0;
        }
        return true;
    }
    std::optional<PulsePlaybackStream> Pulse::moveApplicationToApplicationPassthrough(const std::string &name)
    {
        moveBackApplicationFromPassthrough();

        std::shared_lock lock(playbackStreamMutex);
        if (playbackStreams.find(name) != playbackStreams.end())
        {
            auto &stream = playbackStreams.at(name);
            currentApplicationPassthrough = stream;
            // NOLINTNEXTLINE
            if (system(("pactl move-sink-input " + std::to_string(stream.id) + " soundux_sink_passthrough >/dev/null")
                           .c_str()) != 0)
            {
                Fancy::fancy.logTime().failure()
                    << "Failed to move application " << name << " to passthrough" << std::endl;
            }
            return *currentApplicationPassthrough;
        }
        Fancy::fancy.logTime().failure() << "Failed to find PulsePlaybackStream with name: " << name << std::endl;
        return std::nullopt;
    }
    bool Pulse::isSwitchOnConnectLoaded()
    {
        return (system("pactl list modules | grep module-switch-on-connect >/dev/null") == 0); // NOLINT
    }
    void Pulse::unloadSwitchOnConnect()
    {
        if (system("pactl unload-module module-switch-on-connect >/dev/null") != 0) // NOLINT
        {
            Fancy::fancy.logTime().warning() << "Failed to unload module-switch-on-connect" << std::endl;
        }
        setup();
    }
    bool Pulse::currentlyPassingthrough()
    {
        return currentApplicationPassthrough.has_value();
    }
} // namespace Soundux::Objects
#endif