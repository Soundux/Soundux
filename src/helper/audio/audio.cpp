#include "audio.hpp"
#include "../../core/global/globals.hpp"
#include "../../ui/ui.hpp"
#include <fancy.hpp>
#include <optional>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace Soundux::Objects
{
    void Audio::setup()
    {
        refreshAudioDevices();
    }
    void Audio::destory()
    {
        stopAll();
    }
    std::optional<PlayingSound> Audio::play(const Objects::Sound &sound,
                                            const std::optional<Objects::AudioDevice> &playbackDevice,
                                            bool shouldNotReport)
    {
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
        auto length_in_pcm_frames = ma_decoder_get_length_in_pcm_frames(decoder);

        config.pUserData = decoder;
        config.dataCallback = data_callback;
        config.sampleRate = decoder->outputSampleRate;
        config.playback.format = decoder->outputFormat;
        config.playback.channels = decoder->outputChannels;
        if (playbackDevice)
        {
            config.playback.pDeviceID = &playbackDevice->raw.id;
        }
        else
        {
            config.playback.pDeviceID = &defaultOutputDevice.raw.id;
        }

        if (ma_device_init(nullptr, &config, device) != MA_SUCCESS)
        {
            Fancy::fancy.logTime().failure() << "Failed to create default playback device" << std::endl;
            return std::nullopt;
        }
        if (ma_device_start(device) != MA_SUCCESS)
        {
            ma_device_uninit(device);
            ma_decoder_uninit(decoder);
            Fancy::fancy.logTime().warning() << "Failed to play sound " << sound.path << std::endl;

            return std::nullopt;
        }

        PlayingSound pSound;
        pSound.sound = sound;
        pSound.rawDevice = device;
        pSound.rawDecoder = decoder;
        pSound.length = length_in_pcm_frames;
        pSound.sampleRate = config.sampleRate;
        pSound.shouldNotReport = shouldNotReport;
        pSound.lengthInMs = static_cast<std::uint64_t>(static_cast<double>(pSound.length) /
                                                       static_cast<double>(config.sampleRate) * 1000);
        pSound.id = ++playingSoundIdCounter;

        soundsMutex.lock();
        playingSounds.insert({device, pSound});
        soundsMutex.unlock();

        Globals::gGui->onSoundPlayed(pSound);

        return pSound;
    }
    void Audio::stopAll()
    {
        std::unique_lock lock(soundsMutex);
        for (const auto &sound : playingSounds)
        {
            lock.unlock();
            ma_device_uninit(sound.second.rawDevice);
            ma_decoder_uninit(sound.second.rawDecoder);
            Globals::gGui->onSoundFinished(sound.second);
            lock.lock();
        }
        playingSounds.clear();
    }
    bool Audio::stop(const std::uint32_t &soundId)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });

        if (sound != playingSounds.end())
        {
            lock.unlock();
            ma_device_uninit(sound->second.rawDevice);
            ma_decoder_uninit(sound->second.rawDecoder);
            lock.lock();

            playingSounds.erase(sound);
            return true;
        }

        Fancy::fancy.logTime().failure() << "Failed to stop sound with id " << soundId << ", sound does not exist"
                                         << std::endl;
        return false;
    }
    std::optional<PlayingSound> Audio::pause(const std::uint32_t &soundId)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });

        if (sound != playingSounds.end())
        {
            if (!sound->second.paused)
            {
                lock.unlock();
                ma_device_stop(sound->second.rawDevice);
                lock.lock();
                sound->second.paused = true;
            }
            return sound->second;
        }

        Fancy::fancy.logTime().failure() << "Failed to pause sound with id " << soundId << ", sound does not exist"
                                         << std::endl;
        return std::nullopt;
    }
    std::optional<PlayingSound> Audio::repeat(const std::uint32_t &soundId, bool shouldRepeat)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });

        if (sound != playingSounds.end())
        {
            sound->second.repeat = shouldRepeat;
            return sound->second;
        }

        Fancy::fancy.logTime().failure() << "Failed to set repeat for sound with id " << soundId
                                         << ", sound does not exist" << std::endl;
        return std::nullopt;
    }
    std::optional<PlayingSound> Audio::resume(const std::uint32_t &soundId)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });

        if (sound != playingSounds.end())
        {
            if (sound->second.paused)
            {
                ma_device_start(sound->second.rawDevice);
                sound->second.paused = false;
            }
            return sound->second;
        }
        Fancy::fancy.logTime().failure() << "Failed to resume sound with id " << soundId << ", sound does not exist"
                                         << std::endl;
        return std::nullopt;
    }
    float Audio::getVolume(const std::string &name)
    {
#if defined(__linux__)
        if (name == sinkAudioDevice.name)
        {
            return Globals::gSettings.remoteVolume;
        }
#endif
        std::shared_lock lock(deviceMutex);
        if (devices.find(name) != devices.end())
        {
            if (devices.at(name).isDefault)
            {
                return Globals::gSettings.localVolume;
            }
            return Globals::gSettings.remoteVolume;
        }
        return 1.f;
    }
    void Audio::onFinished(ma_device *device)
    {
        std::unique_lock lock(soundsMutex);
        if (playingSounds.find(device) != playingSounds.end())
        {
            auto &sound = playingSounds.at(device);
            lock.unlock();
            ma_device_uninit(sound.rawDevice);
            ma_decoder_uninit(sound.rawDecoder);
            Globals::gGui->onSoundFinished(sound);
            lock.lock();

            playingSounds.erase(device);
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Sound finished but is not playing?" << std::endl;
        }
    }
    std::optional<PlayingSound> Audio::getPlayingSound(ma_device *device)
    {
        std::shared_lock lock(soundsMutex);
        if (playingSounds.find(device) != playingSounds.end())
        {
            return playingSounds.at(device);
        }

        return std::nullopt;
    }
    void Audio::onSoundProgressed(ma_device *device, std::uint64_t frames)
    {
        std::unique_lock lock(soundsMutex);
        if (playingSounds.find(device) != playingSounds.end())
        {
            auto &sound = playingSounds.at(device);
            sound.readFrames += frames;
            sound.buffer += frames;

            if (sound.buffer > (sound.sampleRate / 2))
            {
                sound.readInMs = static_cast<std::uint64_t>(
                    (static_cast<double>(sound.readFrames) / static_cast<double>(sound.length)) *
                    static_cast<double>(sound.lengthInMs));
                Globals::gGui->onSoundProgressed(sound);
                sound.buffer = 0;
            }
        }
    }
    void Audio::onSoundSeeked(ma_device *device, std::uint64_t frame)
    {
        std::unique_lock lock(soundsMutex);
        if (playingSounds.find(device) != playingSounds.end())
        {
            auto &sound = playingSounds.at(device);
            sound.shouldSeek = false;
            sound.readFrames = frame;
            sound.readInMs =
                static_cast<std::uint64_t>((static_cast<double>(frame) / static_cast<double>(sound.length)) *
                                           static_cast<double>(sound.lengthInMs));
        }
    }
    std::optional<PlayingSound> Audio::seek(const std::uint32_t &soundId, std::uint64_t position)
    {
        std::unique_lock lock(soundsMutex);
        auto sound = std::find_if(playingSounds.begin(), playingSounds.end(),
                                  [soundId](const auto &sound) { return sound.second.id == soundId; });
        if (sound != playingSounds.end())
        {
            sound->second.seekTo = static_cast<std::uint64_t>(
                (static_cast<double>(position) / static_cast<double>(sound->second.lengthInMs)) *
                static_cast<double>(sound->second.length));
            sound->second.shouldSeek = true;

            auto rtn = sound->second;
            rtn.readFrames = rtn.seekTo;
            rtn.readInMs =
                static_cast<std::uint64_t>((static_cast<double>(rtn.seekTo) / static_cast<double>(rtn.length)) *
                                           static_cast<double>(rtn.lengthInMs));

            return rtn;
        }
        Fancy::fancy.logTime().failure() << "Failed to seek sound with id " << soundId << ", sound does not exist"
                                         << std::endl;
        return std::nullopt;
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
        auto sound = Globals::gAudio.getPlayingSound(device);

        if (sound && sound->shouldSeek)
        {
            ma_decoder_seek_to_pcm_frame(decoder, sound->seekTo);
            Globals::gAudio.onSoundSeeked(device, sound->seekTo);
        }
        if (sound && !sound->shouldNotReport && readFrames > 0)
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
    std::vector<AudioDevice> Audio::fetchAudioDevices()
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
            Fancy::fancy.logTime().failure() << "Failed to initialize context" << std::endl;
            return {};
        }

        ma_device_info *pPlayBackDeviceInfos{};
        ma_uint32 deviceCount{};

        ma_result result = ma_context_get_devices(&context, &pPlayBackDeviceInfos, &deviceCount, nullptr, nullptr);
        if (result != MA_SUCCESS)
        {
            Fancy::fancy.logTime().failure() << "Failed to get playback devices!" << std::endl;
            return {};
        }

        std::vector<AudioDevice> playBackDevices;
        for (unsigned int i = 0; deviceCount > i; i++)
        {
            auto &rawDevice = pPlayBackDeviceInfos[i];

            AudioDevice device;
            device.raw = rawDevice;
            device.name = rawDevice.name;
            device.isDefault = rawDevice.name == defaultName;

            playBackDevices.emplace_back(device);
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
            rtn.emplace_back(sound.second);
        }

        return rtn;
    }
    std::vector<AudioDevice> Audio::getAudioDevices()
    {
        std::shared_lock lock(deviceMutex);

        std::vector<AudioDevice> rtn;
        for (const auto &device : devices)
        {
            rtn.emplace_back(device.second);
        }
        return rtn;
    }
    std::optional<std::reference_wrapper<AudioDevice>> Audio::getAudioDevice(const std::string &name)
    {
        std::shared_lock lock(deviceMutex);
        if (devices.find(name) != devices.end())
        {
            return devices.at(name);
        }
        Fancy::fancy.logTime().failure() << "Failed to receive AudioDevice with name " << name
                                         << ", AudioDevice does not exist" << std::endl;
        return std::nullopt;
    }
    void Audio::refreshAudioDevices()
    {
        auto deviceList = fetchAudioDevices();
        std::unique_lock lock(deviceMutex);

        for (const auto &device : deviceList)
        {
            devices.insert({device.name, device});
            if (device.isDefault)
            {
                defaultOutputDevice = devices.at(device.name);
            }
#if defined(__linux__)
            if (device.name == "soundux_sink")
            {
                sinkAudioDevice = devices.at(device.name);
            }
#endif
        }
    }
} // namespace Soundux::Objects