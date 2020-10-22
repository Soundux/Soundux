#pragma once
#include <miniaudio.h>
#include <safe_ptr.h>
#include <exception>
#include <cstdint>
#include <iostream>
#include <vector>
#include <thread>
#include <map>

namespace Soundux
{
    namespace Playback
    {
        namespace internal
        {
            struct PlayingDevice
            {
                std::uint64_t id;
                ma_device *device;
                ma_decoder *decoder;
                bool finished = false;
            };

            inline std::map<std::string, float> usedDevices; // Can't use id as key because its not map compliant
            inline sf::safe_ptr<std::vector<PlayingDevice>> currentlyPlayingDevices;

            void data_callback(ma_device *device, void *output, [[maybe_unused]] const void *input,
                               std::uint32_t frameCount);
        } // namespace internal

        std::vector<ma_device_info> getPlaybackDevices();

        void setVolume(const ma_device_info &deviceInfo, float volume);

        std::uint64_t playAudio(const std::string &file, const ma_device_info &deviceInfo);

        void stop(const std::uint64_t &deviceId);

        void stopAllAudio();

        namespace internal
        {
            inline std::atomic<bool> killGarbageCollector = false;
            inline auto garbageCollector = [] {
                std::thread collector([] {
                    while (!killGarbageCollector.load())
                    {
                        for (int i = 0; currentlyPlayingDevices->size() > i; i++)
                        {
                            auto &device = currentlyPlayingDevices->at(i);
                            if (device.finished)
                            {
                                stop(device.id);
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                });
                collector.detach();
                return collector;
            }();
        } // namespace internal
        inline void destroy()
        {
            internal::killGarbageCollector.store(true);
            internal::garbageCollector.join();
        }
    } // namespace Playback
} // namespace Soundux