#pragma once
#include "../../core/global/objects.hpp"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <miniaudio.h>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>

namespace Soundux
{
    namespace Objects
    {
        struct AudioDevice
        {
            ma_device_info raw;
            std::string name;
            bool isDefault;
            float volume;
        };
        struct PlayingSound
        {
            ma_device *rawDevice;
            ma_decoder *rawDecoder;
            long long length = 0;
            long long seekTo = 0;
            int lengthInSeconds = 0;
            long long readFrames = 0;

            AudioDevice device;
            std::uint32_t id;
            Sound sound;
            bool paused = false;
            bool repeat = false;
        };
        class Audio
        {
            std::shared_mutex soundsMutex;
            std::map<ma_device *, PlayingSound> playingSounds;

            std::shared_mutex deviceMutex;
            std::map<std::string, AudioDevice> devices;

            static void data_callback(ma_device *device, void *output, const void *input, std::uint32_t frameCount);

          public:
            void stopAll();
            void stop(const std::uint32_t &);
            void pause(const std::uint32_t &);
            void resume(const std::uint32_t &);
            void seek(const std::uint32_t &, long long);
            std::optional<PlayingSound> play(const Objects::Sound &,
                                             const std::optional<AudioDevice> &device = std::nullopt);

            void onFinished(ma_device *);
            void onSoundSeeked(ma_device *, long long);
            void onSoundProgressed(ma_device *, long long);
            std::optional<PlayingSound> getSound(ma_device *);

            float getVolume(const std::string &);
            std::vector<AudioDevice> getAudioDevices();
            std::vector<Objects::PlayingSound> getPlayingSounds();

            Audio();
            ~Audio();

            AudioDevice *defaultOutputDevice;
        };
    } // namespace Objects
} // namespace Soundux