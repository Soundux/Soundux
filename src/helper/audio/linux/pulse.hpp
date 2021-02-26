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
            void unloadLeftOverModules();
            void fetchDefaultPulseSource();

            PulseData data;
            std::optional<PulseRecordingStream> currentApplication;
            std::optional<PulsePlaybackStream> currentApplicationPassthrough;

            std::shared_mutex recordingStreamMutex;
            std::map<std::string, PulseRecordingStream> recordingStreams;

            std::shared_mutex playbackStreamMutex;
            std::map<std::string, PulsePlaybackStream> playbackStreams;

          public:
            void setup();
            void destroy();

            void unloadSwitchOnConnect();
            bool isSwitchOnConnectLoaded();

            bool moveBackCurrentApplication();
            bool setDefaultSourceToSoundboardSink();
            bool revertDefaultSourceToOriginal() const;
            bool moveApplicationToSinkMonitor(const std::string &);

            void refreshRecordingStreams();
            std::vector<PulseRecordingStream> getRecordingStreams();

            void refreshPlaybackStreams();
            std::vector<PulsePlaybackStream> getPlaybackStreams();

            bool moveBackApplicationFromPassthrough();
            std::optional<PulsePlaybackStream> moveApplicationToApplicationPassthrough(const std::string &);
        };
    } // namespace Objects
} // namespace Soundux
#endif