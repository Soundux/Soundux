#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace nlohmann
{
    template <typename, typename> struct adl_serializer;
} // namespace nlohmann

namespace Soundux
{
    namespace Objects
    {
        struct AudioDevice;

        enum class ErrorCode : std::uint8_t
        {
            FailedToPlay,
            FailedToSeek,
            FailedToPause,
            FailedToRepeat,
            FailedToResume,
            FailedToMoveToSink,
            SoundNotFound,
            FolderDoesNotExist,
            TabDoesNotExist,
            FaildToSetHotkey,
            FailedToStartPassthrough,
        };

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
            std::vector<int> stopHotkey;
            bool useAsDefaultDevice = false;
            std::uint32_t selectedTab = 0;
            bool allowOverlapping = true;
            bool tabHotkeysOnly = false;
            bool darkTheme = true;
            bool gridView = false;
            std::string output;

            float remoteVolume = 1.f;
            float localVolume = 1.f;
        };
        class Data
        {
            template <typename, typename> friend struct nlohmann::adl_serializer;

          private:
            std::vector<Tab> tabs;

          public:
            int width = 1280, height = 720;
            std::uint32_t soundIdCounter = 0;

            std::vector<Tab> getTabs() const;
            void setTabs(const std::vector<Tab> &);
            std::optional<Tab> setTab(const std::uint32_t &, const Tab &);

            Tab addTab(Tab);
            void removeTabById(const std::uint32_t &);

            std::optional<Tab> getTab(const std::uint32_t &) const;
            std::optional<std::reference_wrapper<Sound>> getSound(const std::uint32_t &);

            Data &operator=(const Data &other);
        };
    } // namespace Objects
} // namespace Soundux