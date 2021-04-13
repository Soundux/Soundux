#pragma once
#include <core/global/objects.hpp>
#include <string>

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