#include <optional>
#if defined(__linux__)
#include "../audio.hpp"
#include "fancy.hpp"
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
        result.push_back(line);
    }
    return result;
};

namespace Soundux::Objects
{
    void Audio::setupSink()
    {
        fetchDefaultPulseSource();

        auto moduleId = exec("pactl load-module module-null-sink sink_name=soundux_sink rate=44100 "
                             "sink_properties=device.description=soundux_sink");
        moduleId.erase(std::remove(moduleId.begin(), moduleId.end(), '\n'), moduleId.end());
        nullSinkModuleId = std::stoi(moduleId);

        auto loopbackId = exec("pactl load-module module-loopback rate=44100 source=\"" + pulseDefaultSource +
                               "\" sink=\"soundux_sink\""); // NOLINT
        loopbackId.erase(std::remove(loopbackId.begin(), loopbackId.end(), '\n'), loopbackId.end());
        loopbackModuleId = std::stoi(loopbackId);

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
                    sinkMonitorId = std::stoi(deviceId);
                    return;
                }
            }
        }

        Fancy::fancy.logTime().failure() << "Failed to find monitor of soundux sink!" << std::endl;
    }
    void Audio::deleteSink() const
    {
        system(("pactl unload-module " + std::to_string(loopbackModuleId)).c_str()); // NOLINT
        system(("pactl unload-module " + std::to_string(nullSinkModuleId)).c_str()); // NOLINT
    }
    void Audio::fetchDefaultPulseSource()
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
                    pulseDefaultSource = match[1];
                    Fancy::fancy.logTime() << "Default Pulse Source was saved: " << pulseDefaultSource << std::endl;
                }
            }
        }
    }
    void Audio::revertDefaultSourceToOriginal()
    {
        if (!pulseDefaultSource.empty())
        {
            system(("pactl set-default-source " + pulseDefaultSource).c_str()); // NOLINT
        }
        else
        {
            Fancy::fancy.logTime().failure()
                << "Failed to revert default source, default source was not set!" << std::endl;
        }
    }
    void Audio::setDefaultSourceToSoundboardSink()
    {
        system("pactl set-default-source soundboard_sink.monitor"); // NOLINT
    }
    void Audio::moveApplicationToSinkMonitor(const PulseRecordingStream &stream)
    {
        moveBackCurrentApplication();
        currentApplication = stream;
        system(("pactl move-source-output " + std::to_string(stream.id) + " soundux_sink.monitor").c_str()); // NOLINT
    }
    void Audio::moveBackCurrentApplication()
    {
        if (currentApplication)
        {
            // NOLINTNEXTLINE
            system(("pactl move-source-output " + std::to_string(currentApplication->id) + " " +
                    currentApplication->source)
                       .c_str());
        }
    }
    void Audio::refreshRecordingStreams()
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
                        fetchedStreams.push_back(stream);
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
        if (stream && std::find(fetchedStreams.begin(), fetchedStreams.end(), stream) == fetchedStreams.end())
        {
            fetchedStreams.push_back(stream);
        }

        std::unique_lock lock(recordingStreamMutex);
        for (const auto &stream : fetchedStreams)
        {
            recordingStreams.insert({stream.name, stream});
        }
    }
    std::vector<PulseRecordingStream> Audio::getRecordingStreams()
    {
        std::shared_lock lock(recordingStreamMutex);
        std::vector<PulseRecordingStream> rtn;
        for (const auto &stream : recordingStreams)
        {
            rtn.push_back(stream.second);
        }
        return rtn;
    }
    std::optional<PulseRecordingStream> Audio::getRecordingStream(const std::string &name)
    {
        std::shared_lock lock(recordingStreamMutex);
        if (recordingStreams.find(name) != recordingStreams.end())
        {
            return recordingStreams.at(name);
        }
        Fancy::fancy.logTime().warning() << "Could not find PulseRecordingStream with name " << name << std::endl;
        return std::nullopt;
    }
} // namespace Soundux::Objects
#endif