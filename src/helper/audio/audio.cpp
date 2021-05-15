#include "audio.hpp"
#include <core/global/globals.hpp>
#include <fancy.hpp>
#if defined(_WIN32)
#include <helper/misc/misc.hpp>
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace Soundux::Objects
{
#if defined(_WIN32)
    using Soundux::Helpers::widen;
#endif

    void Audio::setup()
    {
        for (const auto &device : getAudioDevices())
        {
            if (device.isDefault)
            {
                defaultPlayback = device;
            }
#if defined(__linux__)
            if (device.name == "soundux_sink")
            {
                nullSink = device;
            }
#endif
        }
    }
    void Audio::destroy()
    {
        stopAll();
    }
    std::optional<PlayingSound> Audio::play(const Objects::Sound &sound,
                                            const std::optional<Objects::AudioDevice> &playbackDevice)
    {
        static std::atomic<std::uint64_t> id = 0;

        auto *decoder = new ma_decoder;
#if defined(_WIN32)
        auto res = ma_decoder_init_file_w(widen(sound.path).c_str(), nullptr, decoder);
#else
        auto res = ma_decoder_init_file(sound.path.c_str(), nullptr, decoder);
#endif

        if (res != MA_SUCCESS)
        {
            Fancy::fancy.logTime().failure() << "Failed to create decoder from file: " << sound.path << ", error: " >>
                res << std::endl;
            delete decoder;

            return std::nullopt;
        }

        auto *device = new ma_device;
        auto config = ma_device_config_init(ma_device_type_playback);
        auto length_in_pcm_frames = ma_decoder_get_length_in_pcm_frames(decoder);

        config.dataCallback = data_callback;
        config.sampleRate = decoder->outputSampleRate;
        config.playback.format = decoder->outputFormat;
        config.playback.channels = decoder->outputChannels;

        auto pSound = std::make_shared<PlayingSound>();
        config.pUserData = reinterpret_cast<void *>(static_cast<PlayingSound *>(pSound.get()));

        if (playbackDevice)
        {
            config.playback.pDeviceID = &playbackDevice->raw.id;
        }
        else
        {
            config.playback.pDeviceID = &defaultPlayback.raw.id;
        }

        if (ma_device_init(nullptr, &config, device) != MA_SUCCESS)
        {
            Fancy::fancy.logTime().failure() << "Failed to create device" << std::endl;
            ma_decoder_uninit(decoder);
            delete decoder;
            delete device;

            return std::nullopt;
        }

        if (ma_device_start(device) != MA_SUCCESS)
        {
            ma_device_uninit(device);
            ma_decoder_uninit(decoder);

            delete device;
            delete decoder;

            Fancy::fancy.logTime().warning() << "Failed to play sound " << sound.path << std::endl;

            return std::nullopt;
        }

        auto soundId = ++id;

        pSound->id = soundId;
        pSound->sound = sound;
        pSound->raw.device = device;
        pSound->raw.decoder = decoder;
        pSound->length = length_in_pcm_frames;
        pSound->sampleRate = config.sampleRate;
        pSound->playbackDevice = playbackDevice ? *playbackDevice : defaultPlayback;
        pSound->lengthInMs = static_cast<std::uint64_t>(static_cast<double>(pSound->length) /
                                                        static_cast<double>(config.sampleRate) * 1000);

        playingSoundsMutex.lock();
        playingSounds.emplace(soundId, pSound);
        playingSoundsMutex.unlock();

        return *pSound;
    }
    void Audio::stopAll()
    {
        std::unique_lock lock(playingSoundsMutex);
        while (!playingSounds.empty())
        {
            auto &sound = playingSounds.begin()->second;
            if (sound->raw.device && sound->raw.decoder)
            {
                ma_device_uninit(sound->raw.device);
                ma_decoder_uninit(sound->raw.decoder);
            }

            sound->raw.device = nullptr;
            sound->raw.decoder = nullptr;

            playingSounds.erase(sound->id);
        }
    }
    bool Audio::stop(const std::uint32_t &soundId)
    {
        std::unique_lock lock(playingSoundsMutex);
        if (playingSounds.find(soundId) != playingSounds.end())
        {
            auto &sound = playingSounds.at(soundId);
            if (sound->raw.device && sound->raw.decoder)
            {
                ma_device_uninit(sound->raw.device);
                ma_decoder_uninit(sound->raw.decoder);
            }

            sound->raw.device = nullptr;
            sound->raw.decoder = nullptr;

            playingSounds.erase(sound->id);
            return true;
        }

        Fancy::fancy.logTime().warning() << "Failed to stop sound with id " << soundId << ", sound does not exist"
                                         << std::endl;
        return false;
    }
    std::optional<PlayingSound> Audio::pause(const std::uint32_t &soundId)
    {
        std::unique_lock lock(playingSoundsMutex);
        if (playingSounds.find(soundId) != playingSounds.end())
        {
            auto &sound = playingSounds.at(soundId);

            if (!sound->paused)
            {
                if (ma_device_get_state(sound->raw.device) == MA_STATE_STARTED)
                {
                    ma_device_stop(sound->raw.device);
                }
                sound->paused = true;
            }

            return *sound;
        }

        Fancy::fancy.logTime().warning() << "Failed to pause sound with id " << soundId << ", sound does not exist"
                                         << std::endl;
        return std::nullopt;
    }
    std::optional<PlayingSound> Audio::repeat(const std::uint32_t &soundId, bool shouldRepeat)
    {
        std::unique_lock lock(playingSoundsMutex);
        if (playingSounds.find(soundId) != playingSounds.end())
        {
            auto &sound = playingSounds.at(soundId);
            sound->repeat = shouldRepeat;

            return *sound;
        }

        Fancy::fancy.logTime().warning() << "Failed to set repeat for sound with id " << soundId
                                         << ", sound does not exist" << std::endl;
        return std::nullopt;
    }
    std::optional<PlayingSound> Audio::resume(const std::uint32_t &soundId)
    {
        std::unique_lock lock(playingSoundsMutex);
        if (playingSounds.find(soundId) != playingSounds.end())
        {
            auto &sound = playingSounds.at(soundId);

            if (sound->paused)
            {
                if (ma_device_get_state(sound->raw.device) == MA_STATE_STOPPED)
                {
                    ma_device_start(sound->raw.device);
                }
                sound->paused = false;
            }

            return *sound;
        }

        Fancy::fancy.logTime().warning() << "Failed to resume sound with id " << soundId << ", sound does not exist "
                                         << std::endl;
        return std::nullopt;
    }
    void Audio::onFinished(PlayingSound sound)
    {
        std::unique_lock lock(playingSoundsMutex);
        if (playingSounds.find(sound.id) != playingSounds.end())
        {
            ma_device_uninit(sound.raw.device);
            ma_decoder_uninit(sound.raw.decoder);

            sound.raw.device = nullptr;
            sound.raw.decoder = nullptr;

            Globals::gGui->onSoundFinished(sound);
            playingSounds.erase(sound.id);
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Sound finished but is not playing" << std::endl;
        }
    }
    void Audio::onSoundProgressed(PlayingSound *sound, std::uint64_t frames)
    {
        sound->readFrames += frames;
        sound->buffer += frames;

        if (sound->buffer > (sound->sampleRate / 2))
        {
            sound->readInMs = static_cast<std::uint64_t>(
                (static_cast<double>(sound->readFrames) / static_cast<double>(sound->length)) *
                static_cast<double>(sound->lengthInMs));

            Globals::gGui->onSoundProgressed(*sound);

            sound->buffer = 0;
        }
    }
    void Audio::onSoundSeeked(PlayingSound *sound, std::uint64_t frame)
    {
        sound->shouldSeek = false;
        sound->readFrames = frame;
        sound->readInMs = static_cast<std::uint64_t>((static_cast<double>(frame) / static_cast<double>(sound->length)) *
                                                     static_cast<double>(sound->lengthInMs));
    }
    std::optional<PlayingSound> Audio::seek(const std::uint32_t &soundId, std::uint64_t position)
    {
        std::unique_lock lock(playingSoundsMutex);
        if (playingSounds.find(soundId) != playingSounds.end())
        {
            auto &sound = playingSounds.at(soundId);
            sound->seekTo =
                static_cast<std::uint64_t>((static_cast<double>(position) / static_cast<double>(sound->lengthInMs)) *
                                           static_cast<double>(sound->length));
            sound->shouldSeek = true;

            auto rtn = *sound;
            rtn.readFrames = rtn.seekTo;
            rtn.readInMs =
                static_cast<std::uint64_t>((static_cast<double>(rtn.seekTo) / static_cast<double>(rtn.length)) *
                                           static_cast<double>(rtn.lengthInMs));

            return rtn;
        }

        Fancy::fancy.logTime().warning() << "Failed to seek sound with id " << soundId << ", sound does not exist"
                                         << std::endl;
        return std::nullopt;
    }
    void Audio::data_callback(ma_device *device, void *output, [[maybe_unused]] const void *input,
                              std::uint32_t frameCount)
    {
        auto *sound = reinterpret_cast<PlayingSound *>(device->pUserData);
        if (!sound)
        {
            return;
        }

        if (!sound->raw.decoder)
        {
            return;
        }

        device->masterVolumeFactor =
            sound->playbackDevice.isDefault ? Globals::gSettings.localVolume : Globals::gSettings.remoteVolume;

        auto readFrames = ma_decoder_read_pcm_frames(sound->raw.decoder, output, frameCount);
        if (sound->shouldSeek)
        {
            ma_decoder_seek_to_pcm_frame(sound->raw.decoder, sound->seekTo);
            Globals::gAudio.onSoundSeeked(sound, sound->seekTo);
        }
        if (sound->playbackDevice.isDefault && readFrames > 0)
        {
            Globals::gAudio.onSoundProgressed(sound, readFrames);
        }

        if (readFrames <= 0)
        {
            if (sound->repeat)
            {
                ma_decoder_seek_to_pcm_frame(sound->raw.decoder, 0);
                Globals::gAudio.onSoundSeeked(sound, 0);
            }
            else
            {
                Globals::gQueue.push_unique(reinterpret_cast<std::uintptr_t>(device),
                                            [sound = *sound] { Globals::gAudio.onFinished(sound); });
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
#if defined(_WIN32)
    std::optional<AudioDevice> Audio::getAudioDevice(const std::string &name)
    {
        for (const auto &device : getAudioDevices())
        {
            if (device.name == name)
            {
                return device;
            }
        }
        return std::nullopt;
    }
#endif
    std::vector<PlayingSound> Audio::getPlayingSounds()
    {
        std::lock_guard lock(playingSoundsMutex);

        std::vector<PlayingSound> rtn;
        for (const auto &sound : playingSounds)
        {
            rtn.emplace_back(*sound.second);
        }

        return rtn;
    }
    PlayingSound::PlayingSound(const PlayingSound &other)
    {
        if (&other == this)
        {
            return;
        }

        length = other.length;
        lengthInMs = other.lengthInMs;
        readFrames = other.readFrames;
        sampleRate = other.sampleRate;

        id = other.id;
        sound = other.sound;
        buffer = other.buffer;

        seekTo.store(other.seekTo);
        paused.store(other.paused);
        repeat.store(other.repeat);
        readInMs.store(other.readInMs);
        shouldSeek.store(other.shouldSeek);

        raw.device.store(other.raw.device);
        raw.decoder.store(other.raw.decoder);
        playbackDevice = other.playbackDevice;
    }
    PlayingSound &PlayingSound::operator=(const PlayingSound &other)
    {
        if (&other == this)
        {
            return *this;
        }

        length = other.length;
        lengthInMs = other.lengthInMs;
        readFrames = other.readFrames;
        sampleRate = other.sampleRate;

        id = other.id;
        sound = other.sound;
        buffer = other.buffer;

        seekTo.store(other.seekTo);
        paused.store(other.paused);
        repeat.store(other.repeat);
        readInMs.store(other.readInMs);
        shouldSeek.store(other.shouldSeek);

        raw.device.store(other.raw.device);
        raw.decoder.store(other.raw.decoder);
        playbackDevice = other.playbackDevice;

        return *this;
    }
} // namespace Soundux::Objects