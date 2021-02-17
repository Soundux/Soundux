#pragma once
#include "../core/global/objects.hpp"
#include "../helper/audio/audio.hpp"
#include <cstdint>
#include <queue>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class Window
        {
          protected:
            std::shared_mutex eventMutex;
            std::queue<std::function<void()>> eventQueue;
            virtual void progressEvents();

            virtual void stopSounds();
            virtual void stopSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> playSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> pauseSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> resumeSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> seekSound(const std::uint32_t &, std::uint64_t);

            virtual void changeSettings(const Settings &);

            // TODO(curve):
            // virtual void changeTabOrder(...);
            // virtual void changeLocalVolume(const std::uint8_t &);
            // virtual void changeRemoteVolume(const std::uint8_t &);
            // virtual void changeOutputDevice(const std::uint32_t &);

            virtual std::optional<Tab> addTab();
            virtual std::vector<Sound> refreshTabSounds(const Tab &) const;

          public:
            ~Window();
            virtual void setup();
            virtual void mainLoop() = 0;

            virtual void onSoundPlayed(const PlayingSound &) = 0;
            virtual void onSoundFinished(const PlayingSound &) = 0;
            virtual void onSoundProgressed(const PlayingSound &) = 0;
            virtual void onHotKeyReceived(const std::vector<std::string> &);

            virtual void onEvent(const std::function<void()> &);
        };
    } // namespace Objects
} // namespace Soundux