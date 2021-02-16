#pragma once
#include "../core/global/objects.hpp"
#include "../helper/audio/audio.hpp"
#include <cstdint>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class Window
        {
          protected:
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
            virtual void onSoundPaused(const PlayingSound &) = 0;
            virtual void onSoundSeeked(const PlayingSound &) = 0;
            virtual void onSoundResumed(const PlayingSound &) = 0;
            virtual void onSoundFinished(const PlayingSound &) = 0;
            virtual void onHotKeyReceived(const std::vector<std::string> &);
        };
    } // namespace Objects
} // namespace Soundux