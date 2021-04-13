#pragma once
#include <core/global/objects.hpp>
#include <helper/audio/audio.hpp>
#if defined(__linux__)
#include <helper/audio/linux/pulse.hpp>
#endif
#include <atomic>
#include <cstdint>
#include <queue>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class Window
        {
            friend class Hotkeys;

          protected:
            std::shared_mutex groupedSoundsMutex;
            std::map<std::uint32_t, std::uint32_t> groupedSounds;

            virtual void onAllSoundsFinished();

            virtual void stopSounds();
            virtual void isOnFavorites(bool);
            virtual bool stopSound(const std::uint32_t &);
            virtual std::vector<Tab> removeTab(const std::uint32_t &);
            virtual std::optional<Tab> refreshTab(const std::uint32_t &);
            virtual std::optional<PlayingSound> playSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> pauseSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> resumeSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> repeatSound(const std::uint32_t &, bool);
            virtual std::optional<PlayingSound> seekSound(const std::uint32_t &, std::uint64_t);
            virtual std::optional<Sound> setHotkey(const std::uint32_t &, const std::vector<int> &);

            virtual std::string getHotkeySequence(const std::vector<int> &);

            virtual void changeSettings(const Settings &);

            virtual std::vector<Sound> getFavourites();
            virtual std::vector<Sound> markFavourite(const std::uint32_t &, bool);

            virtual std::optional<Tab> addTab();
            virtual std::vector<Sound> refreshTabSounds(const Tab &) const;
            virtual std::vector<Tab> changeTabOrder(const std::vector<int> &);

#if defined(__linux__)
            virtual std::vector<PulseRecordingStream> getOutputs();
            virtual std::vector<PulsePlaybackStream> getPlayback();

            void stopPassthrough();
            virtual bool startPassthrough(const std::string &);

            virtual void deleteSound(const std::uint32_t &);
#else
            virtual std::vector<AudioDevice> getOutputs();
#endif

          public:
            virtual ~Window();
            virtual void setup();
            virtual void mainLoop() = 0;

            virtual void onError(const ErrorCode &) = 0;
            virtual void onSoundPlayed(
                const PlayingSound &); //* This will be called when a sound is played through a hotkey. PlaySound
                                       //* will be called before this gets called
            virtual void onSoundFinished(const PlayingSound &);
            virtual void onSoundProgressed(const PlayingSound &) = 0;
            virtual void onDownloadProgressed(float, const std::string &) = 0;
            virtual void onHotKeyReceived(const std::vector<int> &);
        };
    } // namespace Objects
} // namespace Soundux