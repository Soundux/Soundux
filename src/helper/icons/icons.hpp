#if defined(__linux__)
#pragma once
#include "forward.hpp"
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class IconFetcher
        {
            LibWnck::Screen *screen;
            std::map<int, std::string> cache;

          private:
            IconFetcher() = default;

            bool setup();
            std::optional<int> getPpid(int pid);

          public:
            static std::unique_ptr<IconFetcher> createInstance();
            std::optional<std::string> getIcon(int pid, bool recursive = true);
        };
    } // namespace Objects
} // namespace Soundux
#endif