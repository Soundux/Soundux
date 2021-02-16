#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct AudioDevice;

        struct Sound
        {
            std::uint32_t id;
            std::string name;
            std::string path;

            std::vector<int> hotkeys;
            std::uint64_t modifiedDate;
        };
        struct Tab
        {
            // TODO(curve) will be used later to move tabs
            std::uint32_t id; //* Equal to index
            std::string name;
            std::string path;

            std::vector<Sound> sounds;
        };

        struct Settings
        {
            // TODO(curve): Adjust deviceSettings & Audio::Devices when UI changes some settings
            std::vector<AudioDevice> deviceSettings;
            std::vector<int> stopHotkey;
            bool allowOverlapping;
            bool tabHotkeysOnly;
            bool darkTheme;
        };
        class Data
        {
          private:
            std::vector<Tab> tabs;

          public:
            int width, height;
            std::string output; // TODO(curve): Make use of `output`
            std::uint32_t soundIdCounter = 0;

            std::vector<Tab> getTabs() const;
            void setTabs(const std::vector<Tab> &);

            Tab addTab(Tab);
            void removeTabById(const std::uint32_t &);

            std::optional<Tab> getTab(const std::uint32_t &) const;
            std::optional<std::reference_wrapper<Sound>> getSound(const std::uint32_t &);
        };
    } // namespace Objects
} // namespace Soundux