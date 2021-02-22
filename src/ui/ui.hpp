#pragma once
#include "../core/global/objects.hpp"
#include "../helper/audio/audio.hpp"
#if defined(__linux__)
#include "../helper/audio/linux/pulse.hpp"
#endif
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
            std::shared_mutex groupedSoundsMutex;
            std::map<std::uint32_t, std::uint32_t> groupedSounds;

            std::shared_mutex eventMutex;
            std::atomic<bool> shouldCheck = false;
            std::queue<std::function<void()>> eventQueue;
            virtual void progressEvents();

            virtual void stopSounds();
            virtual bool stopSound(const std::uint32_t &);
            virtual std::vector<Tab> removeTab(const std::uint32_t &);
            virtual std::optional<Tab> refreshTab(const std::uint32_t &);
            virtual std::optional<PlayingSound> pauseSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> resumeSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> repeatSound(const std::uint32_t &, bool);
            virtual std::optional<PlayingSound> seekSound(const std::uint32_t &, std::uint64_t);
            virtual std::optional<PlayingSound> playSound(const std::uint32_t &, const std::string &);

            virtual void changeSettings(const Settings &);

            // TODO(curve):
            // virtual void changeTabOrder(...);
            // virtual void changeLocalVolume(const std::uint8_t &);
            // virtual void changeRemoteVolume(const std::uint8_t &);

            virtual std::optional<Tab> addTab();
            virtual std::vector<Sound> refreshTabSounds(const Tab &) const;

#if defined(__linux__)
            virtual std::vector<PulseRecordingStream> getOutput();
            virtual std::vector<PulsePlaybackStream> getPlayback();

            void stopPassthrough();
            virtual std::optional<PulsePlaybackStream> startPassthrough(const std::string &, const std::string &);
#else
            virtual std::vector<AudioDevice> getOutput();
#endif

          public:
            ~Window();
            virtual void setup();
            virtual void mainLoop() = 0;

            virtual void onSoundPlayed(const PlayingSound &) = 0;
            virtual void onSoundFinished(const PlayingSound &);
            virtual void onSoundProgressed(const PlayingSound &) = 0;
            virtual void onHotKeyReceived(const std::vector<int> &);

            virtual void onEvent(const std::function<void()> &);
        };
    } // namespace Objects
} // namespace Soundux