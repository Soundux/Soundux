#pragma once
#include <cstdint>
#include <functional>
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
            FailedToSetHotkey,
            FailedToStartPassthrough,
            FailedToMoveBack,
            FailedToMoveBackPassthrough,
            FailedToRevertDefaultSource,
            FailedToSetDefaultSource,
            YtdlNotFound,
            YtdlInvalidUrl,
            YtdlInvalidJson,
            YtdlInformationUnknown,
            FailedToDelete
        };

        enum class SortMode : std::uint8_t
        {
            ModifiedDate_Ascending,
            ModifiedDate_Descending,
            Alphabetical_Ascending,
            Alphabetical_Descending,
        };

        enum class Theme : std::uint8_t
        {
            System,
            Dark,
            Light
        };

        enum class ViewMode : std::uint8_t
        {
            List,
            Grid,
            EmulatedLaunchpad,
        };

        struct Sound
        {
            std::uint32_t id;
            std::string name;
            std::string path;
            bool isFavorite = false;

            std::vector<int> hotkeys;
            std::uint64_t modifiedDate;
        };
        struct Tab
        {
            std::uint32_t id; //* Equal to index
            std::string name;
            std::string path;

            std::vector<Sound> sounds;
        };

        struct Settings
        {
            SortMode sortMode = SortMode::ModifiedDate_Descending;
            ViewMode viewMode = ViewMode::List;
            Theme theme = Theme::System;

            std::vector<int> pushToTalkKeys;
            std::vector<int> stopHotkey;

            std::uint32_t selectedTab = 0;
            std::string output;

            float remoteVolume = 1.f;
            float localVolume = 0.5f;
            bool syncVolumes = false;

            bool useAsDefaultDevice = false;
            bool muteDuringPlayback = false;
            bool allowOverlapping = true;
            bool minimizeToTray = false;
            bool tabHotkeysOnly = false;
            bool deleteToTrash = true;
        };
        class Data
        {
            template <typename, typename> friend struct nlohmann::adl_serializer;

          private:
            std::vector<Tab> tabs;

          public:
            bool isOnFavorites = false;
            int width = 1280, height = 720;
            std::uint32_t soundIdCounter = 0;

            std::vector<Tab> getTabs() const;
            void setTabs(const std::vector<Tab> &);
            std::optional<Tab> setTab(const std::uint32_t &, const Tab &);

            Tab addTab(Tab);
            void removeTabById(const std::uint32_t &);

            std::optional<Tab> getTab(const std::uint32_t &) const;
            std::optional<std::reference_wrapper<Sound>> getSound(const std::uint32_t &);

            std::vector<Sound> getFavorites();
            std::vector<std::uint32_t> getFavoriteIds();
            void markFavorite(const std::uint32_t &, bool);

            void set(const Data &other);
            Data &operator=(const Data &other) = delete;
        };
    } // namespace Objects
} // namespace Soundux