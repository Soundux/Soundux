#if defined(__linux__) && defined(USE_WNCK)
#pragma once
#include <libwnck-3.0/libwnck/libwnck.h>
#include <map>
#include <optional>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class IconFetcher
        {
            WnckScreen *screen;
            std::map<int, std::string> cache;

          public:
            void setup();
            std::optional<std::string> getIcon(int pid, bool recursive = true);
        };
    } // namespace Objects
} // namespace Soundux
#endif