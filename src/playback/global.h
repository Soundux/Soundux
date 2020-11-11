#pragma once
#include <miniaudio.h>
#include <functional>
#include <exception>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
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
                std::string soundPath;
            };
            struct DefaultDevice
            {
                std::string name;
            };

            inline std::map<std::string, float> usedDevices;

            inline std::mutex playingDeviceMutex;
            inline std::vector<PlayingDevice> currentlyPlayingDevices;

            void data_callback(ma_device *device, void *output, const void *input, std::uint32_t frameCount);
        } // namespace internal

        Playback::internal::DefaultDevice getDefaultCaptureDevice();
        Playback::internal::DefaultDevice getDefaultPlaybackDevice();

        inline auto defaultCapture = getDefaultCaptureDevice();
        inline auto defaultPlayback = getDefaultPlaybackDevice();

        std::vector<ma_device_info> getCaptureDevices();
        std::vector<ma_device_info> getPlaybackDevices();

        void setVolume(const std::string &deviceName, float volume);

        std::uint64_t playAudio(const std::string &file);
        std::uint64_t playAudio(const std::string &file, const ma_device_info &deviceInfo);

        void stop(const std::uint64_t &deviceId);
        inline std::function<void(const internal::PlayingDevice &)> stopCallback = [](const auto &) {};

        void pause(const std::uint64_t &deviceId);

        void resume(const std::uint64_t &deviceId);

        void stopAllAudio();

        namespace internal
        {
            inline std::atomic<bool> killGarbageCollector = false;

            inline std::map<ma_device *, bool> deviceClearQueue;

            inline auto garbageCollector = [] {
                std::thread collector([] {
                    while (!killGarbageCollector.load())
                    {
                        for (auto sound = deviceClearQueue.begin(); deviceClearQueue.end() != sound; ++sound)
                        {
                            if (sound->second)
                            {
                                playingDeviceMutex.lock();
                                for (unsigned int i = 0; currentlyPlayingDevices.size() > i; i++)
                                {
                                    auto &device = currentlyPlayingDevices.at(i);
                                    if (device.device != sound->first)
                                        continue;
                                    stop(device.id);
                                }
                                playingDeviceMutex.unlock();
                                deviceClearQueue.erase(sound);
                                break;
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                });
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