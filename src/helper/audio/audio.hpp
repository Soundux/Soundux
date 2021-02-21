#pragma once
#include "../../core/global/objects.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <miniaudio.h>
#include <optional>
#include <shared_mutex>
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
            float volume;
        };
#if defined(__linux__)
        struct PulseRecordingStream
        {
            std::uint32_t id;
            std::string name;
            std::string driver;
            std::string source;
            std::string resampleMethod;

            operator bool() const
            {
                return driver == "protocol-native.c" && resampleMethod != "peaks";
            }
        };
#endif
        struct PlayingSound
        {
            ma_device *rawDevice;
            ma_decoder *rawDecoder;
            std::uint64_t seekTo = 0;
            std::uint64_t length = 0;
            std::uint64_t readInMs = 0;
            std::uint64_t lengthInMs = 0;
            std::uint64_t readFrames = 0;
            std::uint64_t sampleRate = 0;
            bool shouldNotReport = false;

            std::uint64_t buffer = 0;

            std::uint32_t id;
            Sound sound;
            bool paused = false;
            bool repeat = false;
            bool shouldSeek = false;
        };
        class Audio
        {
            std::uint32_t playingSoundIdCounter;

            std::shared_mutex soundsMutex;
            std::map<ma_device *, PlayingSound> playingSounds;

            std::shared_mutex deviceMutex;
            std::map<std::string, AudioDevice> devices;

            std::vector<AudioDevice> fetchAudioDevices();
            static void data_callback(ma_device *device, void *output, const void *input, std::uint32_t frameCount);

#if defined(__linux__)
            void setupSink();
            void deleteSink() const;
            void fetchDefaultPulseSource();

            std::uint32_t sinkMonitorId;
            std::uint32_t nullSinkModuleId;
            std::uint32_t loopbackModuleId;
            std::string pulseDefaultSource;
            std::optional<PulseRecordingStream> currentApplication;

            std::shared_mutex recordingStreamMutex;
            std::map<std::string, PulseRecordingStream> recordingStreams;
#endif

            void onFinished(ma_device *);
            void onSoundSeeked(ma_device *, std::uint64_t);
            void onSoundProgressed(ma_device *, std::uint64_t);

            float getVolume(const std::string &);
            std::optional<PlayingSound> getPlayingSound(ma_device *);

          public:
            void stopAll();
            bool stop(const std::uint32_t &);
            std::optional<PlayingSound> pause(const std::uint32_t &);
            std::optional<PlayingSound> resume(const std::uint32_t &);
            std::optional<PlayingSound> seek(const std::uint32_t &, std::uint64_t);
            std::optional<PlayingSound> play(const Objects::Sound &, const std::optional<AudioDevice> & = std::nullopt,
                                             bool = false);

            std::vector<Objects::PlayingSound> getPlayingSounds();

            void refreshAudioDevices();
            std::vector<AudioDevice> getAudioDevices();
            std::optional<std::reference_wrapper<AudioDevice>> getAudioDevice(const std::string &);

#if defined(__linux__)
            AudioDevice *sinkAudioDevice;
            void refreshRecordingStreams();
            void moveBackCurrentApplication();
            void revertDefaultSourceToOriginal();
            void setDefaultSourceToSoundboardSink();
            void moveApplicationToSinkMonitor(const PulseRecordingStream &);

            std::vector<PulseRecordingStream> getRecordingStreams();
            std::optional<PulseRecordingStream> getRecordingStream(const std::string &);
#endif

            Audio();
            ~Audio();

            AudioDevice *defaultOutputDevice;
        };
    } // namespace Objects
} // namespace Soundux