#pragma once
#include <core/enums/enums.hpp>
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
            Enums::SortMode sortMode = Enums::SortMode::ModifiedDate_Descending;
        };
    } // namespace Objects
} // namespace Soundux