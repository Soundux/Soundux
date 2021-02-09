#if defined(__linux__) && __has_include(<X11/Xlib.h>)
#include "../hotkeys.hpp"
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <chrono>
#include <cstdlib>
#include <fancy.hpp>
#include <thread>

using namespace std::chrono_literals;

namespace Soundux::Objects
{
    Display *display;
    void Hotkeys::listen()
    {
        auto *displayenv = std::getenv("DISPLAY");
        auto *x11Display = XOpenDisplay(displayenv);

        if (!x11Display)
        {
            Fancy::fancy.logTime().warning() << "DISPLAY is not set, defaulting to :0" << std::endl;
            if (!(x11Display = XOpenDisplay(":0")))
            {
                Fancy::fancy.logTime().failure() << "Could not open X11 Display" << std::endl;
                return;
            }
        }
        display = x11Display;

        int major_op = 0, event_rtn = 0, ext_rtn = 0;
        if (!XQueryExtension(display, "XInputExtension", &major_op, &event_rtn, &ext_rtn))
        {
            Fancy::fancy.logTime().failure() << "Failed to find XInputExtension" << std::endl;
            return;
        }

        Window root = DefaultRootWindow(display); // NOLINT

        XIEventMask mask;
        mask.deviceid = XIAllMasterDevices;
        mask.mask_len = XIMaskLen(XI_LASTEVENT);
        mask.mask = static_cast<unsigned char *>(calloc(mask.mask_len, sizeof(char)));

        XISetMask(mask.mask, XI_RawKeyPress);
        XISetMask(mask.mask, XI_RawKeyRelease);
        XISelectEvents(display, root, &mask, 1);

        XSync(display, 0);
        free(mask.mask);

        while (!kill)
        {
            if (notify && XPending(display))
            {
                XEvent event;
                XNextEvent(display, &event);
                auto *cookie = reinterpret_cast<XGenericEventCookie *>(&event.xcookie);

                if (XGetEventData(display, cookie) && cookie->type == GenericEvent && cookie->extension == major_op &&
                    (cookie->evtype == XI_RawKeyPress || cookie->evtype == XI_RawKeyRelease))
                {
                    auto *data = reinterpret_cast<XIRawEvent *>(cookie->data);
                    auto key = data->detail;

                    if (cookie->evtype == XI_RawKeyPress)
                    {
                        onKeyDown(key);
                    }
                    else
                    {
                        onKeyUp(key);
                    }
                }
            }
            else
            {
                std::this_thread::sleep_for(100ms);
            }
        }
    }

    std::string Hotkeys::getKeyName(const int &key)
    {
        KeySym s = XkbKeycodeToKeysym(display, key, 0, 0);
        if (NoSymbol == s)
        {
            return "Unknown";
        }

        auto *str = XKeysymToString(s);
        if (str == nullptr)
        {
            return "Unknown";
        }

        return str;
    }
} // namespace Soundux::Objects

#endif