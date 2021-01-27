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

            static void save();
            static void load();
            static std::string path;
        };
    } // namespace Objects
} // namespace Soundux