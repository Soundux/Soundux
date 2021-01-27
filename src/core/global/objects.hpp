#pragma once
#include <map>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct Sound
        {
            std::uint32_t id;
            std::string name;
            std::string path;

            std::uint64_t modifiedDate;
            std::vector<std::size_t> hotKeys;
        };
        struct Tab
        {
            std::string name;
            std::string path;

            std::vector<Sound> sounds;
        };

        struct Settings
        {
            std::vector<std::uint32_t> stopHotkey;
            std::map<std::string, float> volumes;
            bool allowOverlapping;
            bool tabHotKeysOnly;
            bool darkTheme;
        };
        struct Data
        {
            int width, height;
            std::uint32_t output;
            std::vector<Tab> tabs;
            std::uint32_t soundIdCounter;
        };
    } // namespace Objects
} // namespace Soundux