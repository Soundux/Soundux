#if defined(__linux__) && __has_include(<X11/Xlib.h>)
#include "../hotkeys.hpp"
#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
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
        auto *displayenv = std::getenv("DISPLAY"); // NOLINT
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
        else
        {
            Fancy::fancy.logTime().message() << "Using DISPLAY " << displayenv << std::endl;
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
        XISetMask(mask.mask, XI_RawButtonPress);
        XISetMask(mask.mask, XI_RawButtonRelease);
        XISelectEvents(display, root, &mask, 1);

        XSync(display, 0);
        free(mask.mask);

        while (!kill)
        {
            if (XPending(display) != 0)
            {
                XEvent event;
                XNextEvent(display, &event);
                auto *cookie = reinterpret_cast<XGenericEventCookie *>(&event.xcookie);

                if (XGetEventData(display, cookie) && cookie->type == GenericEvent && cookie->extension == major_op &&
                    (cookie->evtype == XI_RawKeyPress || cookie->evtype == XI_RawKeyRelease ||
                     cookie->evtype == XI_RawButtonPress || cookie->evtype == XI_RawButtonRelease))
                {
                    auto *data = reinterpret_cast<XIRawEvent *>(cookie->data);
                    auto key = data->detail;

                    if (key == 1)
                        continue;

                    if (cookie->evtype == XI_RawKeyPress || cookie->evtype == XI_RawButtonPress)
                    {
                        onKeyDown(key);
                    }
                    else if (cookie->evtype == XI_RawKeyRelease || cookie->evtype == XI_RawButtonRelease)
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
        // TODO(curve): There is no Keysym for the mouse buttons and I couldn't find any way to get the name for the
        // mouse buttons so they'll just be named KEY_1 (1 is the Keycode). Maybe someone will be able to help me but I
        // just can't figure it out

        KeySym s = XkbKeycodeToKeysym(display, key, 0, 0);

        if (s == NoSymbol)
        {
            return "KEY_" + std::to_string(key);
        }

        auto *str = XKeysymToString(s);
        if (str == nullptr)
        {
            return "KEY_" + std::to_string(key);
        }

        return str;
    }

    void Hotkeys::stop()
    {
        kill = true;
        listener.join();
    }

    void Hotkeys::pressKeys(const std::vector<int> &keys)
    {
        keysToPress = keys;
        for (const auto &key : keys)
        {
            XTestFakeKeyEvent(display, key, True, 0);
        }
    }

    void Hotkeys::releaseKeys(const std::vector<int> &keys)
    {
        keysToPress.clear();
        for (const auto &key : keys)
        {
            XTestFakeKeyEvent(display, key, False, 0);
        }
    }
} // namespace Soundux::Objects

#endif