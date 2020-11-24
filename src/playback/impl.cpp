#include <cstdint>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include "global.h"

namespace Soundux
{
    Playback::internal::DefaultDevice Playback::getDefaultPlaybackDevice()
    {
        ma_device device;
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        ma_device_init(0, &deviceConfig, &device);

        Playback::internal::DefaultDevice playbackInfo;
        playbackInfo.name = device.playback.name;

        ma_device_uninit(&device);

        return playbackInfo;
    }
    Playback::internal::DefaultDevice Playback::getDefaultCaptureDevice()
    {
        ma_device device;
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
        ma_device_init(0, &deviceConfig, &device);

        Playback::internal::DefaultDevice captureInfo;
        captureInfo.name = device.capture.name;

        ma_device_uninit(&device);

        return captureInfo;
    }
    std::vector<ma_device_info> Playback::getPlaybackDevices()
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
        for (unsigned int i = 0; deviceCount > i; i++)
        {
            playBackDevices.push_back(pPlayBackDeviceInfos[i]);
        }

        ma_context_uninit(&context);

        return playBackDevices;
    }
    std::vector<ma_device_info> Playback::getCaptureDevices()
    {
        ma_context context;

        if (ma_context_init(0, 0, 0, &context) != MA_SUCCESS)
        {
            std::cerr << "Failed to initialize context" << std::endl;
            return {};
        }

        ma_device_info *pCaptureDeviceInfos;
        ma_uint32 deviceCount;

        ma_result result = ma_context_get_devices(&context, &pCaptureDeviceInfos, &deviceCount, 0, 0);
        if (result != MA_SUCCESS)
        {
            std::cerr << "Failed to get playback devices!" << std::endl;
            return {};
        }

        std::vector<ma_device_info> captureDevices;
        for (unsigned int i = 0; deviceCount > i; i++)
        {
            captureDevices.push_back(pCaptureDeviceInfos[i]);
        }

        ma_context_uninit(&context);

        return captureDevices;
    }
    void Playback::setVolume(const std::string &deviceName, float volume)
    {
        usedDevices[deviceName] = volume;
    }
    std::uint64_t Playback::playAudio(const std::string &file)
    {
        static std::uint64_t counter = 0;

        //? Theoretically we could remove this, but this will result in the defaultPlayBackVolume being 0. This will
        //? only change when the user manually changes this value in the ui where the default value will not match.
        if (usedDevices.find(defaultPlayback.name) == usedDevices.end())
            usedDevices.insert(std::make_pair(defaultPlayback.name, 1.f));

        ma_decoder *decoder = new ma_decoder;
        ma_result result = ma_decoder_init_file(file.c_str(), 0, decoder);

        if (result != MA_SUCCESS)
        {
            std::cerr << "Failed to init decoder" << std::endl;
            return -1;
        }

        ma_device *device = new ma_device;
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = decoder->outputFormat;
        config.playback.channels = decoder->outputChannels;
        config.sampleRate = decoder->outputSampleRate;
        config.dataCallback = internal::data_callback;
        config.pUserData = decoder;

        if (ma_device_init(0, &config, device) != MA_SUCCESS)
        {
            std::cerr << "Failed to open playback device" << std::endl;
            return -1;
        }
        if (ma_device_start(device) != MA_SUCCESS)
        {
            ma_device_uninit(device);
            ma_decoder_uninit(decoder);
            std::cerr << "Failed to start playback device" << std::endl;
            return -1;
        }

        internal::playingDeviceMutex.lock();
        internal::currentlyPlayingDevices.push_back({++counter, device, decoder, file});
        internal::playingDeviceMutex.unlock();

        return counter;
    }
    std::uint64_t Playback::playAudio(const std::string &file, const ma_device_info &deviceInfo)
    {
        static std::uint64_t counter = 0;

        if (usedDevices.find(deviceInfo.name) == usedDevices.end())
            usedDevices.insert(std::make_pair(deviceInfo.name, 1.f));

        ma_decoder *decoder = new ma_decoder;
        ma_result result = ma_decoder_init_file(file.c_str(), 0, decoder);

        if (result != MA_SUCCESS)
        {
            std::cerr << "Failed to init decoder" << std::endl;
            return -1;
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
            return -1;
        }
        if (ma_device_start(device) != MA_SUCCESS)
        {
            ma_device_uninit(device);
            ma_decoder_uninit(decoder);
            std::cerr << "Failed to start playback device" << std::endl;
            return -1;
        }

        internal::playingDeviceMutex.lock();
        internal::currentlyPlayingDevices.push_back({++counter, device, decoder, file});
        internal::playingDeviceMutex.unlock();

        return counter;
    }
    void Playback::stop(const std::uint64_t &deviceId)
    {
        // No need to lock the mutex here, it only gets called when the mutex is locked!
        for (unsigned int i = 0; internal::currentlyPlayingDevices.size() > i; i++)
        {
            auto &device = internal::currentlyPlayingDevices.at(i);
            if (device.id == deviceId)
            {
                if (device.device && device.decoder)
                {
                    stopCallback(device);

                    ma_device_uninit(device.device);
                    ma_decoder_uninit(device.decoder);

                    delete device.device;
                    delete device.decoder;

                    device.device = nullptr;
                    device.decoder = nullptr;
                }

                internal::currentlyPlayingDevices.erase(internal::currentlyPlayingDevices.begin() + i);
                break;
            }
        }
    }
    void Playback::pause(const std::uint64_t &deviceId)
    {
        internal::playingDeviceMutex.lock();
        for (unsigned int i = 0; internal::currentlyPlayingDevices.size() > i; i++)
        {
            auto &device = internal::currentlyPlayingDevices.at(i);

            if (device.id == deviceId)
            {
                ma_device_stop(device.device);
                break;
            }
        }
        internal::playingDeviceMutex.unlock();
    }
    void Playback::resume(const std::uint64_t &deviceId)
    {
        internal::playingDeviceMutex.lock();
        for (unsigned int i = 0; internal::currentlyPlayingDevices.size() > i; i++)
        {
            auto &device = internal::currentlyPlayingDevices.at(i);

            if (device.id == deviceId)
            {
                ma_device_start(device.device);
                break;
            }
        }
        internal::playingDeviceMutex.unlock();
    }
    void Playback::stopAllAudio()
    {
        internal::playingDeviceMutex.lock();
        for (unsigned int i = 0; internal::currentlyPlayingDevices.size() > i; i++)
        {
            auto &device = internal::currentlyPlayingDevices.at(i);
            if (device.device && device.decoder)
            {
                stopCallback(device);

                ma_device_uninit(device.device);
                ma_decoder_uninit(device.decoder);

                delete device.device;
                delete device.decoder;

                device.device = nullptr;
                device.decoder = nullptr;
            }
        }
        internal::currentlyPlayingDevices.clear();
        internal::playingDeviceMutex.unlock();
    }
    void Playback::internal::data_callback(ma_device *device, void *output, [[maybe_unused]] const void *input,
                                           std::uint32_t frameCount)
    {
        ma_decoder *decoder = reinterpret_cast<ma_decoder *>(device->pUserData);
        if (decoder == 0)
            return;

        if (usedDevices.find(device->playback.name) != usedDevices.end())
            device->masterVolumeFactor = usedDevices[device->playback.name];

        auto readFrames = ma_decoder_read_pcm_frames(decoder, output, frameCount);

        if (readFrames <= 0)
        {
            internal::deviceClearQueue[device] = true;
        }
    }
} // namespace Soundux