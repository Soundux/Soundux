#pragma once

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <iostream>
#include <vector>
#include <map>

namespace Soundux
{
    namespace Playback
    {
        namespace internal
        {
            inline std::map<std::string, float> usedDevices; // Can't use id as key because its not map compliant
            inline std::vector<std::pair<ma_device *, ma_decoder *>> currentlyPlayingDevices;

            inline void data_callback(ma_device *device, void *output, [[maybe_unused]] const void *input,
                                      ma_uint32 frameCount)
            {
                ma_decoder *decoder = reinterpret_cast<ma_decoder *>(device->pUserData);
                if (decoder == 0)
                    return;

                if (usedDevices.find(device->playback.name) != usedDevices.end())
                    device->masterVolumeFactor = usedDevices.at(device->playback.name);

                ma_decoder_read_pcm_frames(decoder, output, frameCount);
            }
        } // namespace internal

        inline std::vector<ma_device_info> getPlaybackDevices()
        {
            ma_context context;

            if (ma_context_init(0, 0, 0, &context) != MA_SUCCESS)
            {
                std::cerr << "Failed to initialize context" << std::endl;
                return {};
            }

            ma_device_info *pPlayBackDeviceInfos;
            ma_uint32 deviceCount;

            ma_result result = ma_context_get_devices(&context, &pPlayBackDeviceInfos, &deviceCount, 0, 0);
            if (result != MA_SUCCESS)
            {
                std::cerr << "Failed to get playback devices!" << std::endl;
                return {};
            }

            std::vector<ma_device_info> playBackDevices;
            for (int i = 0; deviceCount > i; i++)
            {
                playBackDevices.push_back(pPlayBackDeviceInfos[i]);
            }

            ma_context_uninit(&context);

            return playBackDevices;
        }
        inline void setVolume(const ma_device_info &deviceInfo, float volume)
        {
            if (internal::usedDevices.find(deviceInfo.name) == internal::usedDevices.end())
            {
                std::cerr << "Device was not found" << std::endl;
                return;
            }
            internal::usedDevices.at(deviceInfo.name) = volume;
        }
        inline void playAudio(const std::string &file, const ma_device_info &deviceInfo)
        {
            if (internal::usedDevices.find(deviceInfo.name) == internal::usedDevices.end())
                internal::usedDevices.insert(std::make_pair(deviceInfo.name, 1.f));

            ma_decoder *decoder = new ma_decoder;
            ma_result result = ma_decoder_init_file(file.c_str(), 0, decoder);

            if (result != MA_SUCCESS)
            {
                std::cerr << "Failed to init decoder" << std::endl;
                return;
            }

            ma_device *device = new ma_device;
            ma_device_config config = ma_device_config_init(ma_device_type_playback);
            config.playback.format = decoder->outputFormat;
            config.playback.channels = decoder->outputChannels;
            config.sampleRate = decoder->outputSampleRate;
            config.dataCallback = internal::data_callback;
            config.playback.pDeviceID = &deviceInfo.id;
            config.pUserData = decoder;

            if (ma_device_init(0, &config, device) != MA_SUCCESS)
            {
                std::cerr << "Failed to open playback device" << std::endl;
                return;
            }
            if (ma_device_start(device) != MA_SUCCESS)
            {
                ma_device_uninit(device);
                ma_decoder_uninit(decoder);
                std::cerr << "Failed to start playback device" << std::endl;
                return;
            }

            internal::currentlyPlayingDevices.push_back(std::make_pair(device, decoder));
        }
        inline void stopAllAudio()
        {
            for (auto &player : internal::currentlyPlayingDevices)
            {
                ma_device_uninit(player.first);
                ma_decoder_uninit(player.second);

                delete player.first;
                delete player.second;
            }
            internal::currentlyPlayingDevices.clear();
        }
    } // namespace Playback
} // namespace Soundux