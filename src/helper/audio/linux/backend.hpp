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
          public:
            AudioBackend() = default;

            virtual void setup();
            virtual void destroy();

            virtual bool useAsDefault();
            virtual bool revertDefault();
            virtual bool muteInput(bool);

            virtual bool stopPassthrough();
            virtual bool isCurrentlyPassingThrough();
            virtual bool passthroughFrom(std::shared_ptr<PlaybackApp>);

            virtual bool stopSoundInput();
            virtual bool inputSoundTo(std::shared_ptr<RecordingApp>);

            virtual std::shared_ptr<PlaybackApp> getPlaybackApp(const std::string &);
            virtual std::shared_ptr<RecordingApp> getRecordingApp(const std::string &);

            virtual std::vector<std::shared_ptr<PlaybackApp>> getPlaybackApps();
            virtual std::vector<std::shared_ptr<RecordingApp>> getRecordingApps();
        };
    } // namespace Objects
} // namespace Soundux
#endif