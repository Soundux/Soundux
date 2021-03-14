#if defined(__linux__)
#include "pulse.hpp"
#include "../../misc/misc.hpp"
#include <fancy.hpp>
#include <mutex>
#include <optional>
#include <regex>
#include <string>

namespace Soundux::Objects
{
    using Soundux::Helpers::exec;
    using Soundux::Helpers::splitByNewLine;

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
        fetchDefaultPulseSource();

        auto originalPlabackStreams = getPlaybackStreams();
        auto originalRecordingStreams = getRecordingStreams();

        if (!setModuleId("pactl load-module module-null-sink sink_name=soundux_sink rate=44100 "
                         "sink_properties=device.description=soundux_sink",
                         data.nullSinkModuleId))
        {
            return;
        }
        if (!setModuleId("pactl load-module module-loopback rate=44100 source=" + data.pulseDefaultSource +
                             " sink=soundux_sink sink_dont_move=true source_dont_move=true",
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
                         "source=soundux_sink_passthrough.monitor sink=soundux_sink source_dont_move=true",
                         data.passthroughLoopbackSinkModuleId))
        {
            return;
        }
        if (!setModuleId(
                "pactl load-module module-loopback source=soundux_sink_passthrough.monitor source_dont_move=true",
                data.passthroughLoopbackMonitorModuleId))
        {
            return;
        }

        fixPlaybackStreams(originalPlabackStreams);
        fixRecordingStreams(originalRecordingStreams);

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
        moveBackCurrentApplications();
        moveBackApplicationsFromPassthrough();
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
    void Pulse::fixRecordingStreams(const std::vector<PulseRecordingStream> &oldStreams)
    {
        auto current = getRecordingStreams();
        for (const auto &app : current)
        {
            auto item = std::find_if(std::begin(oldStreams), std::end(oldStreams),
                                     [&app](const auto &stream) { return stream.id == app.id; });
            if (item != std::end(oldStreams) && item->source != app.source)
            {
                // NOLINTNEXTLINE
                if (system(("pactl move-source-output " + std::to_string(item->id) + " " + item->source).c_str()) == 0)
                {
                    Fancy::fancy.logTime().success()
                        << "Recovered " << item->name << " from left over module to original source: " << item->source
                        << std::endl;
                }
            }
        }
    }
    void Pulse::fixPlaybackStreams(const std::vector<PulsePlaybackStream> &oldStreams)
    {
        auto current = getPlaybackStreams();
        for (const auto &app : current)
        {
            auto item = std::find_if(std::begin(oldStreams), std::end(oldStreams),
                                     [&app](const auto &stream) { return stream.id == app.id; });
            if (item != std::end(oldStreams) && item->sink != app.sink)
            {
                // NOLINTNEXTLINE
                if (system(("pactl move-sink-input " + std::to_string(item->id) + " " + item->sink + " >/dev/null")
                               .c_str()) == 0)
                {
                    Fancy::fancy.logTime().success()
                        << "Recovered " + item->name << " from left over sink to original sink " << item->sink
                        << std::endl;
                }
            }
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
    bool Pulse::moveApplicationsToSinkMonitor(const std::string &streamName)
    {
        auto applications = getRecordingStreams();
        if (currentApplications && currentApplications->first != streamName)
        {
            moveBackCurrentApplications();
        }

        std::vector<PulseRecordingStream> movedStreams;

        for (const auto &app : applications)
        {
            if (app.name == streamName)
            {
                // NOLINTNEXTLINE
                if (system(("pactl move-source-output " + std::to_string(app.id) + " soundux_sink.monitor >/dev/null")
                               .c_str()) == 0)
                {
                    movedStreams.emplace_back(app);
                }
            }
        }

        if (movedStreams.empty())
        {
            Fancy::fancy.logTime().failure()
                << "Failed to find any PulseRecordingStream with name: " << streamName << std::endl;
            return false;
        }

        currentApplications = std::make_pair(streamName, movedStreams);
        return true;
    }
    bool Pulse::moveBackCurrentApplications()
    {
        bool success = true;
        if (currentApplications)
        {
            for (const auto &app : currentApplications->second)
            {
                // NOLINTNEXTLINE
                if (system(("pactl move-source-output " + std::to_string(app.id) + " " + app.source + " >/dev/null")
                               .c_str()) != 0)
                {
                    success = false;
                }
            }
            currentApplications.reset();
        }
        return success; //* Not having anything to moveback should count as a failure
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
            if (stream)
            {
                fetchedStreams.emplace_back(stream);
            }
            return fetchedStreams;
        }
        Fancy::fancy.logTime() << "Failed to get recording streams" << std::endl;
        return {};
    }
    std::vector<PulsePlaybackStream> Pulse::getPlaybackStreams()
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
                        if (stream && stream.name != "soundux")
                        {
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
            if (stream && stream.name != "soundux")
            {
                fetchedStreams.emplace_back(stream);
            }

            return fetchedStreams;
        }
        Fancy::fancy.logTime().failure() << "Failed to get playback streams" << std::endl;
        return {};
    }
    bool Pulse::moveBackApplicationsFromPassthrough()
    {
        bool success = true;
        if (currentApplicationPassthroughs)
        {
            for (const auto &app : currentApplicationPassthroughs->second)
            {
                // clang-format off
                // NOLINTNEXTLINE
                if (system(("pactl move-sink-input " + std::to_string(app.id) + " " + app.sink + " >/dev/null").c_str()) != 0)
                {
                    success = false;
                }
                // clang-format on
            }
            currentApplicationPassthroughs.reset();
        }

        return success;
    }
    bool Pulse::moveApplicationToApplicationPassthrough(const std::string &name)
    {
        moveBackApplicationsFromPassthrough();

        auto apps = getPlaybackStreams();
        std::vector<PulsePlaybackStream> movedStreams;

        for (const auto &app : apps)
        {
            if (app.name == name)
            {
                // NOLINTNEXTLINE
                if (system(("pactl move-sink-input " + std::to_string(app.id) + " soundux_sink_passthrough >/dev/null")
                               .c_str()) == 0)
                {
                    movedStreams.emplace_back(app);
                }
                else
                {
                    Fancy::fancy.logTime().warning()
                        << "Failed to move application " << name << " to passthrough" << std::endl;
                }
            }
        }

        if (!movedStreams.empty())
        {
            currentApplicationPassthroughs = std::make_pair(name, movedStreams);
            return true;
        }

        return false;
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
        return currentApplicationPassthroughs.has_value();
    }
    void Pulse::muteDefaultInput(bool state) const
    {
        // NOLINTNEXTLINE
        if (system(
                ("pactl set-source-mute " + data.pulseDefaultSource + " " + (state ? "true" : "false") + " >/dev/null")
                    .c_str()) != 0)
        {
            Fancy::fancy.warning() << "Failed to set mute state for default input device" << std::endl;
        }
    }
} // namespace Soundux::Objects
#endif