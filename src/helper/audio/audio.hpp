#pragma once
#include <atomic>
#include <core/global/objects.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <miniaudio.h>
#include <mutex>
#include <optional>
#include <string>
#include <var_guard.hpp>

namespace Soundux
{
    namespace Objects
    {
        struct AudioDevice
        {
            ma_device_info raw;
            std::string name;
            bool isDefault;
        };
        struct PlayingSound
        {
            AudioDevice playbackDevice;

            struct
            {
                std::atomic<ma_device *> device;
                std::atomic<ma_decoder *> decoder;
            } raw;

            std::uint64_t length = 0;
            std::uint64_t lengthInMs = 0;
            std::uint64_t readFrames = 0;
            std::uint64_t sampleRate = 0;

            std::atomic<bool> paused = false;
            std::atomic<bool> repeat = false;
            std::atomic<bool> shouldSeek = false;
            std::atomic<std::uint64_t> seekTo = 0;
            std::atomic<std::uint64_t> readInMs = 0;

            Sound sound;
            std::uint32_t id;
            std::uint64_t buffer = 0;

            PlayingSound() = default;
            PlayingSound(const PlayingSound &);
            PlayingSound &operator=(const PlayingSound &other);
        };
        class Audio
        {
            sxl::var_guard<std::map<std::uint32_t, std::shared_ptr<PlayingSound>>, std::recursive_mutex> playingSounds;

            void onFinished(PlayingSound);
            void onSoundSeeked(PlayingSound *, std::uint64_t);
            void onSoundProgressed(PlayingSound *, std::uint64_t);

            static void data_callback(ma_device *device, void *output, const void *input, std::uint32_t frameCount);

          public:
            std::optional<PlayingSound> pause(const std::uint32_t &);
            std::optional<PlayingSound> resume(const std::uint32_t &);
            std::optional<PlayingSound> repeat(const std::uint32_t &, bool);
            std::optional<PlayingSound> seek(const std::uint32_t &, std::uint64_t);
            std::optional<PlayingSound> play(const Objects::Sound &, const std::optional<AudioDevice> & = std::nullopt);

            std::vector<AudioDevice> getAudioDevices();
            std::vector<Objects::PlayingSound> getPlayingSounds();

#if defined(_WIN32)
            std::optional<AudioDevice> getAudioDevice(const std::string &);
#endif

            void setup();
            void destroy();

            void stopAll();
            bool stop(const std::uint32_t &);

#if defined(__linux__)
            std::optional<AudioDevice> nullSink;
#endif
            AudioDevice defaultPlayback;
        };
    } // namespace Objects
} // namespace Soundux