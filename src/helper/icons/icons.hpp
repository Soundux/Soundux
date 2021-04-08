#if defined(__linux__)
#pragma once
#include <gdk/gdk.h>
#include <map>
#include <optional>
#include <string>

namespace Soundux
{
    namespace Lib
    {
        struct WnckScreen;
        struct WnckWindow;
        inline WnckScreen *(*wnckGetDefaultScreen)();
        inline void (*wnckForceUpdate)(WnckScreen *);
        inline int (*wnckGetWindowPID)(WnckWindow *);
        inline GList *(*wnckGetScreenWindows)(WnckScreen *);
        inline GdkPixbuf *(*wnckGetWindowIcon)(WnckWindow *);
    } // namespace Lib
    namespace Objects
    {
        class IconFetcher
        {
            Lib::WnckScreen *screen;
            bool isAvailable = false;
            std::map<int, std::string> cache;

          public:
            void setup();
            std::optional<std::string> getIcon(int pid, bool recursive = true);
        };
    } // namespace Objects
} // namespace Soundux
#endif