#pragma once
#if defined(__linux__)
#include <core/enums/enums.hpp>
#include <memory>
#include <set>
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
            virtual bool setup() = 0;
            AudioBackend() = default;

          public:
            static std::shared_ptr<AudioBackend> createInstance(Enums::BackendType);

          public:
            virtual void destroy() = 0;
            virtual bool useAsDefault() = 0;
            virtual bool revertDefault() = 0;
            virtual bool muteInput(bool) = 0;

            virtual std::set<std::string> currentlyInputApps() = 0;
            virtual std::set<std::string> currentlyPassedThrough() = 0;

            virtual bool stopAllPassthrough() = 0;
            virtual bool stopPassthrough(const std::string &) = 0;
            virtual bool passthroughFrom(const std::string &) = 0;

            virtual bool stopSoundInput() = 0;
            virtual bool inputSoundTo(const std::string &) = 0;

            virtual std::vector<std::shared_ptr<PlaybackApp>> getPlaybackApps() = 0;
            virtual std::vector<std::shared_ptr<RecordingApp>> getRecordingApps() = 0;
        };
    } // namespace Objects
} // namespace Soundux
#endif