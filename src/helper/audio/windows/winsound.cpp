#if defined(_WIN32)
#include "winsound.hpp"
#include <Shlwapi.h>
#include <algorithm>
#include <endpointvolume.h>
#include <fancy.hpp>
#include <functiondiscoverykeys_devpkey.h>
#include <helper/misc/misc.hpp>

namespace Soundux
{
    namespace Objects
    {
        Device::Device(IMMDevice *device)
        {
            this->device = std::shared_ptr<IMMDevice>(device, [](IMMDevice *&ptr) { ptr->Release(); });

            IPropertyStore *store = nullptr;
            if (FAILED(device->OpenPropertyStore(STGM_READ, &store)))
            {
                Fancy::fancy.logTime().warning() << "Failed to open property store of " << device << std::endl;
                return;
            }

            PROPVARIANT friendlyName;
            store->GetValue(PKEY_Device_FriendlyName, &friendlyName);

            if (friendlyName.vt == VT_LPWSTR)
            {
                name = Helpers::narrow(friendlyName.pwszVal);
            }

            PROPVARIANT guidProp;
            store->GetValue(PKEY_Device_GUID, &guidProp);

            if (guidProp.vt == VT_LPWSTR)
            {
                guid = Helpers::narrow(guidProp.pwszVal);
            }
            std::transform(guid.begin(), guid.end(), guid.begin(), [](char c) { return tolower(c); });
        }
        bool WinSound::setup()
        {
            CoInitialize(nullptr);
            IMMDeviceEnumerator *rawEnumerator = nullptr;
            if (!FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                                         __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&rawEnumerator))))
            {
                enumerator = std::shared_ptr<IMMDeviceEnumerator>(
                    rawEnumerator, [](IMMDeviceEnumerator *enumPtr) { enumPtr->Release(); });

                IMMDevice *defaultDevice = nullptr;
                enumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &defaultDevice);
                defaultRecordingDevice = RecordingDevice(defaultDevice);

                if (defaultRecordingDevice)
                {
                    if (defaultRecordingDevice->getName().find("VB-Audio") != std::string::npos)
                    {
                        for (const auto &recordingDevice : getRecordingDevices())
                        {
                            if (recordingDevice.isListeningToDevice())
                            {
                                auto device = getPlaybackDevice(recordingDevice.getDevicePlayingThrough());
                                if (device->getName().find("VB-Audio") != std::string::npos)
                                {
                                    defaultRecordingDevice = recordingDevice;
                                }
                            }
                        }
                        if (defaultRecordingDevice->getName().find("VB-Audio") != std::string::npos)
                        {
                            for (const auto &recordingDevice : getRecordingDevices())
                            {
                                if (recordingDevice.getName().find("VB-Audio") == std::string::npos)
                                {
                                    defaultRecordingDevice = recordingDevice;
                                    break;
                                }
                            }
                        }
                    }
                }

                return true;
            }

            Fancy::fancy.logTime().failure() << "Failed to create enumerator" << std::endl;
            return false;
        }

        std::string Device::getGUID() const
        {
            return guid;
        }
        std::string Device::getName() const
        {
            return name;
        }
        bool RecordingDevice::isMuted() const
        {
            if (!device)
            {
                Fancy::fancy.logTime().failure() << "Failed to get mute state, device was invalid" << std::endl;
                return false;
            }

            IAudioEndpointVolume *endpointVolume = nullptr;
            if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                                        reinterpret_cast<void **>(&endpointVolume))))
            {
                Fancy::fancy.logTime().warning() << "Failed to get muted state of " << name << std::endl;
                return false;
            }

            BOOL isMuted{};
            endpointVolume->GetMute(&isMuted);

            return isMuted;
        }
        bool RecordingDevice::isListeningToDevice() const
        {
            if (!device)
            {
                Fancy::fancy.logTime().failure() << "Failed to get listening state, device was invalid" << std::endl;
                return false;
            }

            IPropertyStore *store = nullptr;
            if (FAILED(device->OpenPropertyStore(STGM_READ, &store)))
            {
                Fancy::fancy.logTime().warning() << "Failed to get listen state of " << name << std::endl;
                return false;
            }

            PROPVARIANT listenProp;
            store->GetValue(PKEY_Device_ListenToThisDevice, &listenProp);

            if (listenProp.vt == VT_BOOL)
            {
                return listenProp.boolVal == -1;
            }

            return false;
        }
        bool RecordingDevice::mute(bool state) const
        {
            if (!device)
            {
                Fancy::fancy.logTime().failure() << "Failed to set mute state, device was invalid" << std::endl;
                return false;
            }

            IAudioEndpointVolume *endpointVolume = nullptr;
            if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                                        reinterpret_cast<void **>(&endpointVolume))))
            {
                Fancy::fancy.logTime().warning() << "Failed to set mute state for " << name << std::endl;
                return false;
            }

            endpointVolume->SetMute(state, nullptr);
            return true;
        }
        bool RecordingDevice::listenToDevice(bool state) const
        {
            if (!device)
            {
                Fancy::fancy.logTime().failure()
                    << "Failed to set listen to this device, device was invalid" << std::endl;
                return false;
            }

            IPropertyStore *store = nullptr;
            if (auto res = device->OpenPropertyStore(STGM_WRITE, &store); FAILED(res))
            {
                if (res == E_ACCESSDENIED)
                {
                    Fancy::fancy.logTime().warning()
                        << "Access Denied: You need Administrator privileges to perfrom this action" << std::endl;
                }
                if (Globals::gGui)
                {
                    Globals::gGui->onAdminRequired();
                }
                Fancy::fancy.logTime().warning() << "Failed to set listen state for " << name << std::endl;
                return false;
            }

            PROPVARIANT listenProp;
            listenProp.vt = VT_BOOL;
            listenProp.boolVal = state ? -1 : 0;

            store->SetValue(PKEY_Device_ListenToThisDevice, listenProp);
            return true;
        }
        bool RecordingDevice::playbackThrough(const PlaybackDevice &destination) const
        {
            if (!device)
            {
                Fancy::fancy.logTime().failure() << "Failed to set destination, device was invalid" << std::endl;
                return false;
            }

            IPropertyStore *store = nullptr;
            if (!FAILED(device->OpenPropertyStore(STGM_WRITE, &store)))
            {
                PROPVARIANT listenProp;
                listenProp.vt = VT_LPWSTR;

                auto destVal = Helpers::widen("{0.0.0.00000000}." + destination.getGUID());
                auto *destValRaw = new wchar_t[destVal.size() + 1];
                StrCpyW(destValRaw, destVal.c_str());
                listenProp.pwszVal = destValRaw;

                bool success = true;
                if (auto res = store->SetValue(PKEY_Device_PlaybackThrough, listenProp); FAILED(res))
                {
                    if (res == E_ACCESSDENIED)
                    {
                        Fancy::fancy.logTime().warning()
                            << "Access Denied: You need Administrator privileges to perfrom this action" << std::endl;
                        if (Globals::gGui)
                        {
                            Globals::gGui->onAdminRequired();
                        }
                    }
                    Fancy::fancy.logTime().warning() << "Failed to write destination for " << name << std::endl;
                    success = false;
                }

                delete[] destValRaw;
                return success;
            }

            Fancy::fancy.logTime().warning() << "Failed to set destination for " << name << std::endl;
            return false;
        }
        std::string RecordingDevice::getDevicePlayingThrough() const
        {
            if (!device)
            {
                Fancy::fancy.logTime().failure() << "Failed to get destination, device was invalid" << std::endl;
                return "";
            }

            IPropertyStore *store = nullptr;
            if (!FAILED(device->OpenPropertyStore(STGM_READ, &store)))
            {
                PROPVARIANT listenProp;
                if (!FAILED(store->GetValue(PKEY_Device_PlaybackThrough, &listenProp)))
                {
                    if (listenProp.vt != VT_LPWSTR)
                    {
                        return "";
                    }

                    auto prop = Helpers::narrow(listenProp.pwszVal);
                    prop = prop.substr(prop.find_first_of('}') + 2);

                    return prop;
                }
            }

            Fancy::fancy.logTime().warning() << "Failed to get destination for " << name << std::endl;
            return "";
        }
        std::shared_ptr<WinSound> WinSound::createInstance()
        {
            auto instance = std::shared_ptr<WinSound>(new WinSound()); // NOLINT
            if (instance->setup())
            {
                return instance;
            }

            return nullptr;
        }
        std::vector<RecordingDevice> WinSound::getRecordingDevices()
        {
            IMMDeviceCollection *devices = nullptr;
            enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &devices);

            std::uint32_t deviceCount = 0;
            devices->GetCount(&deviceCount);

            std::vector<RecordingDevice> rtn;
            for (std::uint32_t i = 0; deviceCount > i; i++)
            {
                IMMDevice *device = nullptr;
                devices->Item(i, &device);

                rtn.emplace_back(RecordingDevice(device));
            }

            return rtn;
        }
        std::vector<PlaybackDevice> WinSound::getPlaybackDevices()
        {
            IMMDeviceCollection *devices = nullptr;
            enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);

            std::uint32_t deviceCount = 0;
            devices->GetCount(&deviceCount);

            std::vector<PlaybackDevice> rtn;
            for (std::uint32_t i = 0; deviceCount > i; i++)
            {
                IMMDevice *device = nullptr;
                devices->Item(i, &device);

                rtn.emplace_back(PlaybackDevice(device));
            }

            return rtn;
        }
        std::optional<RecordingDevice> WinSound::getRecordingDevice(const std::string &guid)
        {
            std::string lowerGuid = guid;
            std::transform(lowerGuid.begin(), lowerGuid.end(), lowerGuid.begin(), [](char c) { return tolower(c); });

            for (auto &device : getRecordingDevices())
            {
                std::string deviceGuid = device.getGUID();

                if (lowerGuid == deviceGuid)
                {
                    return device;
                }
            }

            return std::nullopt;
        }
        std::optional<PlaybackDevice> WinSound::getPlaybackDevice(const std::string &guid)
        {
            std::string lowerGuid = guid;
            std::transform(lowerGuid.begin(), lowerGuid.end(), lowerGuid.begin(), [](char c) { return tolower(c); });
            for (const auto &device : getPlaybackDevices())
            {
                std::string deviceGuid = device.getGUID();

                if (lowerGuid == deviceGuid)
                {
                    return device;
                }
            }

            return std::nullopt;
        }
        bool WinSound::isVBCableProperlySetup()
        {
            for (const auto &recordingDevice : getRecordingDevices())
            {
                if (recordingDevice.isListeningToDevice())
                {
                    if (recordingDevice.getName().find("VB-Audio") == std::string::npos)
                    {
                        auto playbackDevice = getPlaybackDevice(recordingDevice.getDevicePlayingThrough());
                        if (playbackDevice && playbackDevice->getName().find("VB-Audio") != std::string::npos)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }
        bool WinSound::setupVBCable(const std::optional<RecordingDevice> &deviceOverride)
        {
            defaultRecordingDevice = deviceOverride;
            if (isVBCableProperlySetup())
            {
                return true;
            }

            if (defaultRecordingDevice && defaultRecordingDevice->getName().find("VB-Audio") == std::string::npos)
            {
                if (defaultRecordingDevice->listenToDevice(true))
                {
                    for (const auto &playbackDevice : getPlaybackDevices())
                    {
                        if (playbackDevice.getName().find("VB-Audio") != std::string::npos)
                        {
                            if (defaultRecordingDevice->playbackThrough(playbackDevice))
                            {
                                return true;
                            }
                        }
                    }
                }
            }

            return false;
        }
        std::optional<RecordingDevice> WinSound::getMic()
        {
            return defaultRecordingDevice;
        }
    } // namespace Objects
} // namespace Soundux
#endif