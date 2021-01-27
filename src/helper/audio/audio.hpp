#pragma once
#include "../../core/global/objects.hpp"
#include <map>
#include <memory>
#include <miniaudio.h>
#include <mutex>
#include <optional>
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
            bool isInput;
            float volume;
        };
        struct PlayingSound
        {
            struct
            {
                ma_device *device;
                ma_decoder *decoder;
            } raw;

            AudioDevice device;
            std::uint32_t id;
            Sound sound;
        };
        class Audio
        {
          private:
            std::thread garbageCollector;
            std::mutex playingSoundsMutex;
            std::map<std::uint32_t, Objects::PlayingSound> playingSounds;
            // void data_callback(ma_device *device, void *output, const void *input, std::uint32_t frameCount);

          public:
            std::map<std::string, Objects::AudioDevice> devices;       // TODO(curve): Should get populated on startup
            std::shared_ptr<Objects::AudioDevice> defaultOutputDevice; // ! Is a pointer to an item of `devices`

            void stopSound(); //* Stops all sounds
            void stopSound(const std::uint32_t &);
            void pauseSound(const std::uint32_t &);
            void resumeSound(const std::uint32_t &);
            Objects::PlayingSound playSound(const Objects::Sound &,
                                            const std::optional<Objects::AudioDevice> &device = std::nullopt);

            std::vector<Objects::PlayingSound> getPlayingSounds();

            Audio();  // TODO(curve): Should start garbageCollector
            ~Audio(); // TODO(curve): Shoukd kill garbageCollector
        };
    } // namespace Objects
} // namespace Soundux