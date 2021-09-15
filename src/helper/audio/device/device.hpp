#pragma once
#include <miniaudio.h>
#include <optional>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct AudioDevice
        {
            bool isDefault;
            std::string name;
            ma_device_info raw;

          public:
            static std::vector<AudioDevice> getDevices();
            static std::optional<AudioDevice> getDeviceByName(const std::string &);
        };
    } // namespace Objects
} // namespace Soundux