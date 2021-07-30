#pragma once
#include <core/enums/enums.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct Key;

        struct Sound
        {
            std::uint32_t id;
            std::string name;
            std::string path;
            bool isFavorite = false;

            std::vector<Key> hotkeys;
            std::uint64_t modifiedDate;

            std::optional<int> localVolume;
            std::optional<int> remoteVolume;
        };

        struct Tab
        {
            std::uint32_t id; //* Equal to index
            std::string name;
            std::string path;

            std::vector<Sound> sounds;
            Enums::SortMode sortMode = Enums::SortMode::ModifiedDate_Descending;
        };
    } // namespace Objects
} // namespace Soundux