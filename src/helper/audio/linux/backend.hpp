#pragma once
#if defined(__linux__)
#include <memory>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct RecordingApp
        {
            std::string name;
            std::string application;

            virtual ~RecordingApp() = default;
        };
        struct PlaybackApp
        {
            std::string name;
            std::string application;

            virtual ~PlaybackApp() = default;
        };

        class AudioBackend
        {
          protected:
            AudioBackend() = default;

          public:
            virtual void setup() = 0;
            virtual void destroy() = 0;

            virtual bool useAsDefault() = 0;
            virtual bool revertDefault() = 0;
            virtual bool muteInput(bool) = 0;

            virtual bool stopPassthrough() = 0;
            virtual bool isCurrentlyPassingThrough() = 0;
            virtual bool passthroughFrom(std::shared_ptr<PlaybackApp>) = 0;

            virtual bool stopSoundInput() = 0;
            virtual bool inputSoundTo(std::shared_ptr<RecordingApp>) = 0;

            virtual std::shared_ptr<PlaybackApp> getPlaybackApp(const std::string &) = 0;
            virtual std::shared_ptr<RecordingApp> getRecordingApp(const std::string &) = 0;

            virtual std::vector<std::shared_ptr<PlaybackApp>> getPlaybackApps() = 0;
            virtual std::vector<std::shared_ptr<RecordingApp>> getRecordingApps() = 0;
        };
    } // namespace Objects
} // namespace Soundux
#endif