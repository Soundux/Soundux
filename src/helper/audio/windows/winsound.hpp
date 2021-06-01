#pragma once
#if defined(_WIN32)
#include <fancy.hpp>
#include <mmdeviceapi.h>
#include <optional>
#include <string>
#include <vector>

/*
 * Huge Credits to https://github.com/EinfachEinAlex
 * for investing time into finding out which GUIDs are responsible for "Listen to this Device" and "Playback through
 * device" as well as finding out how to do this in general!
 */

namespace Soundux
{
    namespace Objects
    {
        inline PROPERTYKEY PKEY_Device_GUID{
            GUID{0x1DA5D803, 0xD492, 0x4EDD, {0x8c, 0x23, 0xE0, 0xC0, 0xFF, 0xEE, 0x7F, 0x0E}}, 4};

        inline PROPERTYKEY PKEY_Device_PlaybackThrough{
            GUID{0x24dbb0fc, 0x9311, 0x4b3d, {0x9c, 0xf0, 0x18, 0xff, 0x15, 0x56, 0x39, 0xd4}}, 0};

        inline PROPERTYKEY PKEY_Device_ListenToThisDevice{
            GUID{0x24dbb0fc, 0x9311, 0x4b3d, {0x9c, 0xf0, 0x18, 0xff, 0x15, 0x56, 0x39, 0xd4}}, 1};

        class Device
        {
          protected:
            std::string name;
            std::string guid;
            std::shared_ptr<IMMDevice> device;

          public:
            Device() = default;
            Device(IMMDevice *);

            std::string getName() const;
            std::string getGUID() const;
        };

        class PlaybackDevice : public Device
        {
            using Device::Device;
        };

        class RecordingDevice : public Device
        {
          public:
            using Device::Device;

            bool isMuted() const;
            bool isListeningToDevice() const;
            std::string getDevicePlayingThrough() const;

            void mute(bool) const;
            void listenToDevice(bool) const;
            void playbackThrough(const PlaybackDevice &) const;
        };

        class WinSound
        {
            bool setup();
            std::shared_ptr<IMMDeviceEnumerator> enumerator;

          public:
            static std::shared_ptr<WinSound> createInstance();

            std::vector<PlaybackDevice> getPlaybackDevices();
            std::vector<RecordingDevice> getRecordingDevices();

            std::optional<PlaybackDevice> getPlaybackDevice(const std::string &);
            std::optional<RecordingDevice> getRecordingDevice(const std::string &);
        };
    } // namespace Objects
} // namespace Soundux
#endif