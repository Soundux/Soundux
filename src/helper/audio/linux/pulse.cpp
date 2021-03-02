#if defined(__linux__)
#include "pulse.hpp"
#include "fancy.hpp"
#include <optional>
#include <regex>
#include <sstream>

auto exec(const std::string &command)
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
auto splitByNewLine(const std::string &str)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    for (std::string line; std::getline(ss, line, '\n');)
    {
        result.emplace_back(line);
    }
    return result;
};

namespace Soundux::Objects
{
    void Pulse::setup()
    {
        unloadLeftOverModules();
        refreshPlaybackStreams();
        refreshRecordingStreams();
        fetchDefaultPulseSource();

        auto moduleId = exec("pactl load-module module-null-sink sink_name=soundux_sink rate=44100 "
                             "sink_properties=device.description=soundux_sink");
        moduleId.erase(std::remove(moduleId.begin(), moduleId.end(), '\n'), moduleId.end());
        data.nullSinkModuleId = std::stoi(moduleId);

        auto loopbackId = exec("pactl load-module module-loopback rate=44100 source=" + data.pulseDefaultSource +
                               " sink=soundux_sink sink_dont_move=true"); // NOLINT
        loopbackId.erase(std::remove(loopbackId.begin(), loopbackId.end(), '\n'), loopbackId.end());
        data.loopbackModuleId = std::stoi(loopbackId);

        auto passThroughId = exec("pactl load-module module-null-sink sink_name=soundux_sink_passthrough rate=44100 "
                                  "sink_properties=device.description=soundux_sink_passthrough");
        passThroughId.erase(std::remove(passThroughId.begin(), passThroughId.end(), '\n'), passThroughId.end());
        data.passthroughModuleId = std::stoi(passThroughId);

        auto passThroughSink = exec("pactl load-module module-loopback latency_msec=1 "
                                    "source=soundux_sink_passthrough.monitor sink=soundux_sink");
        passThroughSink.erase(std::remove(passThroughSink.begin(), passThroughSink.end(), '\n'), passThroughSink.end());
        data.passthroughLoopbackSinkModuleId = std::stoi(passThroughSink);

        auto passThroughLoopback = exec("pactl load-module module-loopback source=soundux_sink_passthrough.monitor");
        passThroughLoopback.erase(std::remove(passThroughLoopback.begin(), passThroughLoopback.end(), '\n'),
                                  passThroughLoopback.end());
        data.passthroughLoopbackMonitorModuleId = std::stoi(passThroughLoopback);

        refreshPlaybackStreams(true);
        refreshRecordingStreams(true);

        static const std::regex sourceRegex(R"rgx((.*#(\d+))$|(Name: (.+)))rgx");
        auto sources = exec("LC_ALL=C pactl list sources");
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
    void Pulse::destroy()
    {
        moveBackCurrentApplication();
        moveBackApplicationFromPassthrough();
        revertDefaultSourceToOriginal();

        if (data.loopbackModuleId != 0)
            system(("pactl unload-module " + std::to_string(data.loopbackModuleId)).c_str()); // NOLINT
        if (data.nullSinkModuleId != 0)
            system(("pactl unload-module " + std::to_string(data.nullSinkModuleId)).c_str()); // NOLINT

        if (data.passthroughModuleId != 0)
            system(("pactl unload-module " + std::to_string(data.passthroughModuleId)).c_str()); // NOLINT
        if (data.passthroughLoopbackSinkModuleId != 0)
            // NOLINTNEXTLINE
            system(("pactl unload-module " + std::to_string(data.passthroughLoopbackSinkModuleId)).c_str());
        if (data.passthroughLoopbackMonitorModuleId != 0)
            // NOLINTNEXTLINE
            system(("pactl unload-module " + std::to_string(data.passthroughLoopbackMonitorModuleId)).c_str());
    }
    void Pulse::fetchDefaultPulseSource()
    {
        auto info = exec("LC_ALL=C pactl info");
        static const std::regex defaultDeviceRegex("^Default Source: (.+)$");

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
            }
        }
    }
    bool Pulse::revertDefaultSourceToOriginal() const
    {
        if (!data.pulseDefaultSource.empty())
        {
            if (system(("pactl set-default-source " + data.pulseDefaultSource + " &>/dev/null").c_str()) != 0) // NOLINT
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
        return system("pactl set-default-source soundux_sink.monitor &>/dev/null") == 0; // NOLINT
    }
    bool Pulse::moveApplicationToSinkMonitor(const std::string &streamName)
    {
        moveBackCurrentApplication();
        refreshRecordingStreams();

        std::shared_lock lock(recordingStreamMutex);
        if (recordingStreams.find(streamName) != recordingStreams.end())
        {
            auto &stream = recordingStreams.at(streamName);
            currentApplication = stream;
            // NOLINTNEXTLINE
            return system(
                       ("pactl move-source-output " + std::to_string(stream.id) + " soundux_sink.monitor &>/dev/null")
                           .c_str()) == 0;
        }
        Fancy::fancy.logTime().failure() << "Failed to find PulseRecordingStream with name: " << streamName
                                         << std::endl;
        return false;
    }
    bool Pulse::moveBackCurrentApplication()
    {
        if (currentApplication)
        {
            // NOLINTNEXTLINE
            return system(("pactl move-source-output " + std::to_string(currentApplication->id) + " " +
                           currentApplication->source + " &>/dev/null")
                              .c_str()) == 0;
        }
        return true; //* Not having anything to moveback should count as a failure
    }
    void Pulse::refreshRecordingStreams(const bool &fix)
    {
        auto sourceList = exec("LC_ALL=C pactl list source-outputs");

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
                            stream.source = recordingStreams.at(stream.name).source;
                            if (fix)
                            {
                                // NOLINTNEXTLINE
                                system(("pactl move-source-output " + std::to_string(stream.id) + " " + stream.source)
                                           .c_str());
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
    void Pulse::refreshPlaybackStreams(const bool &fix)
    {
        auto sourceList = exec("LC_ALL=C pactl list sink-inputs");

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
                            stream.sink = playbackStreams.at(stream.name).sink;
                            if (fix)
                            {
                                // NOLINTNEXTLINE
                                system(("pactl move-sink-input " + std::to_string(stream.id) + " " + stream.sink +
                                        " &>/dev/null")
                                           .c_str());
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
    void Pulse::unloadLeftOverModules()
    {
        auto loadedModules = exec("LC_ALL=C pactl list modules");
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
                    system(("pactl unload-module " + currentModuleId).c_str()); // NOLINT
                }
            }
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
                           currentApplicationPassthrough->sink + " &>/dev/null")
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
            system(("pactl move-sink-input " + std::to_string(stream.id) + " soundux_sink_passthrough &>/dev/null")
                       .c_str());
            return *currentApplicationPassthrough;
        }
        Fancy::fancy.logTime().failure() << "Failed to find PulsePlaybackStream with name: " << name << std::endl;
        return std::nullopt;
    }
    bool Pulse::isSwitchOnConnectLoaded()
    {
        return (system("pactl list modules | grep module-switch-on-connect &>/dev/null") == 0); // NOLINT
    }
    void Pulse::unloadSwitchOnConnect()
    {
        system("pactl unload-module module-switch-on-connect &>/dev/null"); // NOLINT
    }
    bool Pulse::currentlyPassingthrough()
    {
        return currentApplicationPassthrough.has_value();
    }
} // namespace Soundux::Objects
#endif