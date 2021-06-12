#if defined(__linux__)
#include "x11.hpp"
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <chrono>
#include <core/hotkeys/keys.hpp>
#include <fancy.hpp>
#include <thread>

namespace Soundux::Objects
{
    void X11::setup()
    {
        Hotkeys::setup();
        listener = std::thread([this] { listen(); });
    }

    void X11::listen()
    {
        auto *displayEnv = std::getenv("DISPLAY"); // NOLINT
        auto *x11Display = XOpenDisplay(displayEnv);

        if (!x11Display)
        {
            Fancy::fancy.logTime().warning() << "DISPLAY is not set, defaulting to :0" << std::endl;
            if (!(x11Display = XOpenDisplay(":0")))
            {
                Fancy::fancy.logTime().failure() << "Could not open X11 Display" << std::endl;
            }
        }
        else
        {
            Fancy::fancy.logTime().message() << "Using DISPLAY " << displayEnv << std::endl;
        }

        display = x11Display;
        int event_rtn = 0, ext_rtn = 0;
        if (!XQueryExtension(display, "XInputExtension", &major_op, &event_rtn, &ext_rtn))
        {
            Fancy::fancy.logTime().failure() << "Failed to find XInputExtension" << std::endl;
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

                if (XGetEventData(display, cookie) && cookie->type == GenericEvent && cookie->extension == major_op)
                {
                    if (cookie->evtype == XI_RawKeyPress || cookie->evtype == XI_RawKeyRelease)
                    {
                        auto *data = reinterpret_cast<XIRawEvent *>(cookie->data);
                        auto key = data->detail;

                        Key pressedKey;
                        pressedKey.key = key;
                        pressedKey.type = Enums::KeyType::Keyboard;

                        if (cookie->evtype == XI_RawKeyPress)
                        {
                            onKeyDown(pressedKey);
                        }
                        else
                        {
                            onKeyUp(pressedKey);
                        }
                    }
                    else if (cookie->evtype == XI_RawButtonPress || cookie->evtype == XI_RawButtonRelease)
                    {
                        auto *data = reinterpret_cast<XIRawEvent *>(cookie->data);
                        auto button = data->detail;

                        if (button != 1)
                        {
                            Key pressedButton;
                            pressedButton.key = button;
                            pressedButton.type = Enums::KeyType::Mouse;

                            if (cookie->evtype == XI_RawButtonPress)
                            {
                                onKeyDown(pressedButton);
                            }
                            else
                            {
                                onKeyUp(pressedButton);
                            }
                        }
                    }
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    std::string X11::getKeyName(const Key &key)
    {
        if (!Hotkeys::getKeyName(key).empty())
        {
            return Hotkeys::getKeyName(key);
        }

        if (key.type == Enums::KeyType::Keyboard)
        {
            KeySym keySym = XkbKeycodeToKeysym(display, key.key, 0, 0);

            if (keySym == NoSymbol)
            {
                return "KEY_" + std::to_string(key.key);
            }

            auto *str = XKeysymToString(keySym);
            if (!str)
            {
                return "KEY_" + std::to_string(key.key);
            }

            return str;
        }

        if (key.type == Enums::KeyType::Mouse)
        {
            return "MOUSE_" + std::to_string(key.key);
        }

        return "";
    }

    void X11::pressKeys(const std::vector<Key> &keys)
    {
        for (const auto &key : keys)
        {
            XTestFakeKeyEvent(display, key.key, True, 0);
        }
    }
    void X11::releaseKeys(const std::vector<Key> &keys)
    {
        for (const auto &key : keys)
        {
            XTestFakeKeyEvent(display, key.key, False, 0);
        }
    }
    X11::~X11()
    {
        kill = true;
        listener.join();
    }
} // namespace Soundux::Objects
#endif