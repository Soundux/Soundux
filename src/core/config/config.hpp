#pragma once
#include "../global/objects.hpp"
#include <map>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct Config
        {
            Data data;
            Settings settings;

            void save();
            void load();
            static const std::string path;
        };
    } // namespace Objects
} // namespace Soundux