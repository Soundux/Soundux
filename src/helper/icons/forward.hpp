#pragma once
#include <gdk/gdk.h>

namespace Soundux
{
    namespace LibWnck
    {
        struct Screen;
        struct Window;

        inline Screen *(*getDefaultScreen)();
        inline void (*forceUpdate)(Screen *);
        inline int (*getWindowPID)(Window *);
        inline GList *(*getScreenWindows)(Screen *);
        inline GdkPixbuf *(*getWindowIcon)(Window *);

        bool setup();
    } // namespace LibWnck
} // namespace Soundux