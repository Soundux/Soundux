#pragma once
#if defined(__linux__)
#include <cstdint>
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct PulseRecordingStream
        {
            std::uint32_t id;
            std::string name;
            std::string driver;
            std::string source;
            std::string resampleMethod;

            operator bool() const
            {
                return driver == "protocol-native.c" && resampleMethod != "peaks";
            }
        };
        struct PulsePlaybackStream
        {
            std::uint32_t id;
            std::string sink;
            std::string name;
            std::string driver;

            operator bool() const
            {
                return driver == "protocol-native.c";
            }
        };
        struct PulseData
        {
            std::uint32_t sinkMonitorId;
            std::string pulseDefaultSource;
            std::uint32_t nullSinkModuleId = 0;
            std::uint32_t loopbackModuleId = 0;

            std::uint32_t passthroughModuleId = 0;
            std::uint32_t passthroughLoopbackSinkModuleId = 0;
            std::uint32_t passthroughLoopbackMonitorModuleId = 0;
        };
        class Pulse
        {
            void fixLeftOvers();
            void unloadLeftOverModules();
            void fetchDefaultPulseSource();
            bool setModuleId(const std::string &, std::uint32_t &);

            PulseData data;
            std::optional<std::pair<std::string, std::vector<PulseRecordingStream>>> currentApplications;
            std::optional<std::pair<std::string, std::vector<PulsePlaybackStream>>> currentApplicationPassthroughs;

          public:
            void setup();
            void destroy();

            void unloadSwitchOnConnect();
            bool isSwitchOnConnectLoaded();

            bool moveBackCurrentApplications();
            bool revertDefaultSourceToOriginal();
            bool setDefaultSourceToSoundboardSink();
            bool moveApplicationsToSinkMonitor(const std::string &);

            std::vector<PulseRecordingStream> getRecordingStreams();
            void fixRecordingStreams(const std::vector<PulseRecordingStream> &);

            std::vector<PulsePlaybackStream> getPlaybackStreams();
            void fixPlaybackStreams(const std::vector<PulsePlaybackStream> &);

            bool currentlyPassingthrough();
            bool moveBackApplicationsFromPassthrough();
            bool moveApplicationToApplicationPassthrough(const std::string &);
        };
    } // namespace Objects
} // namespace Soundux
#endif