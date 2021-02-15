#include "audio.hpp"
#include "../../core/global/globals.hpp"
#include "../../ui/ui.hpp"
#include <fancy.hpp>
#include <iostream>
#include <optional>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace Soundux::Objects
{
    Audio::Audio()
    {
        auto deviceList = getAudioDevices();
        std::unique_lock lock(deviceMutex);

        for (const auto &device : deviceList)
        {
            devices.insert({device.name, device});
            if (device.isDefault)
            {
                defaultOutputDevice = &devices.at(device.name);
            }
        }

        for (const auto &device : Globals::gSettings.deviceSettings)
        {
            if (devices.find(device.name) != devices.end())
            {
                devices.at(device.name).volume = device.volume;
            }
        }
    }
    Audio::~Audio()
    {
        stopAll();
    }
    std::optional<PlayingSound> Audio::play(const Objects::Sound &sound,
                                            const std::optional<Objects::AudioDevice> &playbackDevice)
    {
        if (!Globals::gSettings.allowOverlapping)
        {
            stopAll();
        }

        auto *decoder = new ma_decoder;
        auto res = ma_decoder_init_file(sound.path.c_str(), nullptr, decoder);

        if (res != MA_SUCCESS)
        {
            Fancy::fancy.logTime().logTime().failure()
                << "Failed to create decoder from file: " << sound.path << std::endl;
            return std::nullopt;
        }

        auto *device = new ma_device;
        auto config = ma_device_config_init(ma_device_type_playback);

        config.pUserData = decoder;
        config.dataCallback = data_callback;
        config.sampleRate = decoder->outputSampleRate;
        config.playback.format = decoder->outputFormat;
        config.playback.channels = decoder->outputChannels;

        if (ma_device_init(nullptr, &config, device) != MA_SUCCESS)
        {
            Fancy::fancy.logTime().failure() << "Failed to create default playback device" << std::endl;
            return std::nullopt;
        }
        if (ma_device_start(device) != MA_SUCCESS)
        {
            ma_device_uninit(device);
            ma_decoder_uninit(decoder);
            // TODO(curve): Call some sort of on finished
            Fancy::fancy.logTime().warning() << "Failed to play sound " << sound.path << std::endl;

            return std::nullopt;
        }

        PlayingSound pSound;
        pSound.sound = sound;
        pSound.rawDevice = device;
        pSound.rawDecoder = decoder;
        pSound.length = ma_decoder_get_length_in_pcm_frames(decoder);
        pSound.lengthInSeconds = static_cast<int>(pSound.length / config.sampleRate);
        pSound.id = ++Globals::gData.soundIdCounter;

        if (playbackDevice)
        {
            pSound.device = *playbackDevice;
        }
        else
        {
            pSound.device = *defaultOutputDevice;
        }

        std::unique_lock lock(soundsMutex);
        playingSounds.insert({device, pSound});

        return pSound;
    }
    void Audio::stopAll()
    {
        std::unique_lock lock(soundsMutex);
        for (const auto &sound : playingSounds)
        {
            auto *decoder = static_cast<ma_decoder *>(sound.second.rawDevice->pUserData);
            ma_device_uninit(sound.second.rawDevice);
            ma_decoder_uninit(decoder);
        }
        playingSounds.clear();
    }
    void Audio::stop(const std::uint32_t &soundId)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });

        if (sound != playingSounds.end())
        {
            auto *decoder = reinterpret_cast<ma_decoder *>(sound->second.rawDevice->pUserData);

            ma_device_uninit(sound->second.rawDevice);
            ma_decoder_uninit(decoder);
        }
        else
        {
            Fancy::fancy.logTime().failure()
                << "Failed to stop sound with id " << soundId << ", sound does not exist" << std::endl;
        }
    }
    void Audio::pause(const std::uint32_t &soundId)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });

        if (sound != playingSounds.end())
        {
            auto *decoder = reinterpret_cast<ma_decoder *>(sound->second.rawDevice->pUserData);

            ma_device_stop(sound->second.rawDevice);
            sound->second.paused = true;
        }
        else
        {
            Fancy::fancy.logTime().failure()
                << "Failed to pause sound with id " << soundId << ", sound does not exist" << std::endl;
        }
    }
    void Audio::resume(const std::uint32_t &soundId)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });

        if (sound != playingSounds.end())
        {
            auto *decoder = reinterpret_cast<ma_decoder *>(sound->second.rawDevice->pUserData);

            ma_device_start(sound->second.rawDevice);
            sound->second.paused = false;
        }
        else
        {
            Fancy::fancy.logTime().failure()
                << "Failed to resume sound with id " << soundId << ", sound does not exist" << std::endl;
        }
    }
    float Audio::getVolume(const std::string &name)
    {
        std::shared_lock lock(deviceMutex);
        if (devices.find(name) != devices.end())
        {
            return devices[name].volume;
        }
        return 1.f;
    }
    void Audio::onFinished(ma_device *device)
    {
        std::unique_lock lock(soundsMutex);
        if (playingSounds.find(device) != playingSounds.end())
        {
            auto &sound = playingSounds.at(device);
            ma_device_uninit(sound.rawDevice);
            ma_decoder_uninit(sound.rawDecoder);

            // TODO(curve): Implement in UI
            // Globals::gGui->onSoundFinished(sound->second);

            playingSounds.erase(device);
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Sound finished but is not playing?" << std::endl;
        }
    }
    std::optional<PlayingSound> Audio::getSound(ma_device *device)
    {
        std::shared_lock lock(deviceMutex);
        if (playingSounds.find(device) != playingSounds.end())
        {
            return playingSounds.at(device);
        }

        return std::nullopt;
    }
    void Audio::onSoundProgressed(ma_device *device, long long frames)
    {
        std::unique_lock lock(soundsMutex);
        if (playingSounds.find(device) != playingSounds.end())
        {
            auto &sound = playingSounds.at(device);
            sound.readFrames += frames;
        }
    }
    void Audio::onSoundSeeked(ma_device *device, long long frame)
    {
        std::unique_lock lock(soundsMutex);
        if (playingSounds.find(device) != playingSounds.end())
        {
            playingSounds.at(device).seekTo = 0;
            playingSounds.at(device).readFrames = frame;
        }
    }
    void Audio::seek(const std::uint32_t &soundId, long long position)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });
        if (sound != playingSounds.end())
        {
            sound->second.seekTo = position;
        }
    }
    void Audio::data_callback(ma_device *device, void *output, [[maybe_unused]] const void *input,
                              std::uint32_t frameCount)
    {
        auto *decoder = reinterpret_cast<ma_decoder *>(device->pUserData);
        if (decoder == nullptr)
        {
            return;
        }

        device->masterVolumeFactor = Globals::gAudio.getVolume(device->playback.name);
        auto readFrames = ma_decoder_read_pcm_frames(decoder, output, frameCount);
        auto sound = Globals::gAudio.getSound(device);

        if (sound && sound->seekTo != 0)
        {
            ma_decoder_seek_to_pcm_frame(decoder, sound->seekTo);
            Globals::gAudio.onSoundSeeked(device, sound->seekTo);
        }
        if (readFrames > 0)
        {
            Globals::gAudio.onSoundProgressed(device, readFrames);
        }

        if (readFrames <= 0)
        {
            if (sound && sound->repeat)
            {
                ma_decoder_seek_to_pcm_frame(decoder, 0);
            }
            else
            {
                Globals::gQueue.push_unique(reinterpret_cast<std::uintptr_t>(device),
                                            [&device] { Globals::gAudio.onFinished(device); });
            }
        }
    }
    std::vector<AudioDevice> Audio::getAudioDevices()
    {
        std::string defaultName;
        {
            ma_device device;
            ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
            ma_device_init(nullptr, &deviceConfig, &device);

            defaultName = device.playback.name;

            ma_device_uninit(&device);
        }

        ma_context context;

        if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS)
        {
            std::cerr << "Failed to initialize context" << std::endl;
            return {};
        }

        ma_device_info *pPlayBackDeviceInfos{};
        ma_uint32 deviceCount{};

        ma_result result = ma_context_get_devices(&context, &pPlayBackDeviceInfos, &deviceCount, nullptr, nullptr);
        if (result != MA_SUCCESS)
        {
            std::cerr << "Failed to get playback devices!" << std::endl;
            return {};
        }

        std::vector<AudioDevice> playBackDevices;
        for (unsigned int i = 0; deviceCount > i; i++)
        {
            auto &rawDevice = pPlayBackDeviceInfos[i];

            AudioDevice device;
            device.volume = 1.f;
            device.raw = rawDevice;
            device.name = rawDevice.name;
            device.isDefault = rawDevice.name == defaultName;

            playBackDevices.push_back(device);
        }

        ma_context_uninit(&context);

        return playBackDevices;
    }
    std::vector<PlayingSound> Audio::getPlayingSounds()
    {
        std::shared_lock lock(soundsMutex);
        std::vector<PlayingSound> rtn;
        for (const auto &sound : playingSounds)
        {
            rtn.push_back(sound.second);
        }

        return rtn;
    }
} // namespace Soundux::Objects