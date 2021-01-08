#pragma once
#include "../config/config.h"
#include <atomic>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <map>
#include <miniaudio.h>
#include <mutex>
#include <thread>
#include <vector>

namespace Soundux
{
    namespace Playback
    {
        namespace internal
        {
            struct PlayingSound
            {
                std::uint64_t id;
                ma_device *device;
                ma_decoder *decoder;
                Config::Sound sound;
            };
            struct DefaultDevice
            {
                std::string name;
            };

            inline std::mutex playingSoundsMutext;
            inline std::vector<PlayingSound> playingSounds;

            void data_callback(ma_device *device, void *output, const void *input, std::uint32_t frameCount);
        } // namespace internal

        inline std::map<std::string, float> usedDevices;

        Playback::internal::DefaultDevice getDefaultCaptureDevice();
        Playback::internal::DefaultDevice getDefaultPlaybackDevice();

        inline auto defaultCapture = getDefaultCaptureDevice();
        inline auto defaultPlayback = getDefaultPlaybackDevice();

        std::vector<ma_device_info> getCaptureDevices();
        std::vector<ma_device_info> getPlaybackDevices();

        std::vector<internal::PlayingSound> getPlayingSounds();

        void setVolume(const std::string &deviceName, float volume);

        std::uint64_t playAudio(const Config::Sound &sound);
        std::uint64_t playAudio(const Config::Sound &sound, const ma_device_info &deviceInfo);

        void stop(const std::uint64_t &deviceId);
        inline std::function<void(const internal::PlayingSound &)> stopCallback = [](const auto &device) {};

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
                                playingSoundsMutext.lock();
                                for (unsigned int i = 0; playingSounds.size() > i; i++)
                                {
                                    auto &device = playingSounds.at(i);
                                    if (device.device != sound->first)
                                    {
                                        continue;
                                    }
                                    stop(device.id);
                                }
                                playingSoundsMutext.unlock();
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