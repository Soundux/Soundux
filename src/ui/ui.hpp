#pragma once
#include <core/objects/settings.hpp>
#include <helper/audio/audio.hpp>
#if defined(__linux__)
#include <helper/audio/linux/backend.hpp>
#endif
#include <atomic>
#include <cstdint>
#include <queue>
#include <string>
#include <var_guard.hpp>

namespace Soundux
{
    namespace Objects
    {
#if defined(__linux__)
        struct IconRecordingApp : public RecordingApp
        {
            std::string appIcon;
            IconRecordingApp(const RecordingApp &);
            ~IconRecordingApp() override = default;
        };
        struct IconPlaybackApp : public PlaybackApp
        {
            std::string appIcon;
            IconPlaybackApp(const PlaybackApp &);
            ~IconPlaybackApp() override = default;
        };
#endif

        class Window
        {
            friend class Hotkeys;

          protected:
            sxl::var_guard<std::map<std::uint32_t, std::uint32_t>> groupedSounds;

            struct
            {
                std::string exit;
                std::string hide;
                std::string show;
                std::string settings;
                std::string tabHotkeys;
                std::string muteDuringPlayback;
            } translations;

          protected:
            virtual void onAllSoundsFinished();

          protected:
            virtual std::vector<Sound> getTabContent(const Tab &) const;

#if defined(__linux__)
            virtual std::vector<std::shared_ptr<IconRecordingApp>> getOutputs();
            virtual std::vector<std::shared_ptr<IconPlaybackApp>> getPlayback();
#else
            virtual std::vector<AudioDevice> getOutputs();
#endif

          protected:
            virtual void setIsOnFavorites(bool);
            virtual bool deleteSound(const std::uint32_t &);
            virtual Settings changeSettings(const Settings &);

#if defined(__linux__)
            void stopPassthrough();
            virtual bool startPassthrough(const std::string &);
#endif

          protected:
            virtual std::optional<Tab> addTab();
            virtual std::vector<Tab> removeTab(const std::uint32_t &);
            virtual std::optional<Tab> refreshTab(const std::uint32_t &);
            virtual std::vector<Tab> changeTabOrder(const std::vector<int> &);

          protected:
            virtual void stopSounds(bool = false);
            virtual bool stopSound(const std::uint32_t &);

          protected:
            virtual std::optional<PlayingSound> playSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> pauseSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> resumeSound(const std::uint32_t &);
            virtual std::optional<PlayingSound> repeatSound(const std::uint32_t &, bool);
            virtual std::optional<PlayingSound> seekSound(const std::uint32_t &, std::uint64_t);

            virtual std::optional<Sound> setHotkey(const std::uint32_t &, const std::vector<int> &);

          public:
            virtual ~Window();
            virtual void setup();
            virtual void mainLoop() = 0;

            virtual void onError(const Enums::ErrorCode &) = 0;
            virtual void onSoundPlayed(const PlayingSound &);
            virtual void onSoundFinished(const PlayingSound &);
            virtual void onHotKeyReceived(const std::vector<int> &);
            virtual void onSoundProgressed(const PlayingSound &) = 0;
            virtual void onDownloadProgressed(float, const std::string &) = 0;
        };
    } // namespace Objects
} // namespace Soundux