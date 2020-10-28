#pragma once
#include <miniaudio.h>
#include <safe_ptr.h>
#include <exception>
#include <iostream>
#include <cstdint>
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
                std::string soundPath;
                bool finished = false;
            };

            inline std::map<std::string, float> usedDevices; // Can't use id as key because its not map compliant
            inline sf::safe_ptr<std::vector<PlayingDevice>> currentlyPlayingDevices;

            void data_callback(ma_device *device, void *output, [[maybe_unused]] const void *input,
                               std::uint32_t frameCount);
        } // namespace internal

        auto getDefaultCaptureDevice();
        auto getDefaultPlaybackDevice();

        std::vector<ma_device_info> getCaptureDevices();
        std::vector<ma_device_info> getPlaybackDevices();

        void setVolume(const ma_device_info &deviceInfo, float volume);

        std::uint64_t playAudio(const std::string &file);
        std::uint64_t playAudio(const std::string &file, const ma_device_info &deviceInfo);

        void stop(const std::uint64_t &deviceId);

        void pause(const std::uint64_t &deviceId);

        void resume(const std::uint64_t &deviceId);

        void stopAllAudio();

        namespace internal
        {
            inline std::atomic<bool> killThreads = false;
            inline sf::safe_ptr<std::vector<std::uint64_t>> stopList;
            inline auto garbageCollector = [] {
                std::thread collector([] {
                    while (!killThreads.load())
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
            inline auto stopThread = [] {
                std::thread _stopThread([] {
                    while (!killThreads.load())
                    {
                        for (int i = 0; stopList->size() > i; i++)
                        {
                            auto &item = stopList->at(i);
                            for (int j = 0; currentlyPlayingDevices->size() > j; j++)
                            {
                                auto &device = currentlyPlayingDevices->at(j);
                                if (device.id == item)
                                {
                                    if (device.device && device.decoder)
                                    {
                                        ma_device_uninit(device.device);
                                        ma_decoder_uninit(device.decoder);

                                        delete device.device;
                                        delete device.decoder;
                                    }

                                    internal::currentlyPlayingDevices->erase(
                                        internal::currentlyPlayingDevices->begin() + i);

                                    break;
                                }
                            }
                        }
                        stopList->clear();

                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                });
                _stopThread.detach();

                return _stopThread;
            }();
        } // namespace internal
        inline void destroy()
        {
            internal::killThreads.store(true);
            internal::garbageCollector.join();
        }
    } // namespace Playback
} // namespace Soundux