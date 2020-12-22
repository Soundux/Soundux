#include <X11/Xlib.h>
#ifdef __linux__
#pragma once
#include "global.h"
#include <X11/XKBlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

namespace Soundux
{
    namespace Hooks
    {
        namespace internal
        {
            inline std::thread keyListener;
            inline std::atomic<bool> killThread = false;
            inline Display *display = []() -> Display * {
                const char *displayenv = std::getenv("DISPLAY");
                Display *x11display = XOpenDisplay(displayenv);

                if (x11display == nullptr)
                {
                    std::cerr << "Failed to get X11-Display with value provided by environment variable(" << displayenv
                              << "), falling back "
                                 "to `:0`"
                              << std::endl;
                    x11display = XOpenDisplay(":0");
                }

                if (x11display == nullptr)
                {
                    std::cerr << "Failed to open X11 Display" << std::endl;
                    return nullptr;
                }
                return x11display;
            }();

            inline void hook()
            {
                int xiOpCode = 0;
                int queryEvent = 0;
                int queryError = 0;

                if (XQueryExtension(display, "XInputExtension", &xiOpCode, &queryEvent, &queryError) == 0)
                {
                    std::cerr << "XInput extension is not aviable" << std::endl;
                    return;
                }

                // Custom context
                {
                    int major = 2;
                    int minor = 0;
                    int queryResult = XIQueryVersion(display, &major, &minor);

                    if (queryResult == BadRequest)
                    {
                        std::cerr << "XI 2.0 support is required - Current Version: " << major << "." << minor
                                  << std::endl;
                        return;
                    }
                    if (queryResult != Success)
                    {
                        std::cerr << "Unknown error" << std::endl;
                        return;
                    }
                }

                Window root = DefaultRootWindow(display);
                XIEventMask mask;
                mask.deviceid = XIAllMasterDevices;
                mask.mask_len = XIMaskLen(XI_LASTEVENT);
                mask.mask = static_cast<unsigned char *>(calloc(mask.mask_len, sizeof(char)));

                XISetMask(mask.mask, XI_RawKeyPress);
                XISetMask(mask.mask, XI_RawKeyRelease);
                XISelectEvents(display, root, &mask, 1);
                XSync(display, 0);
                free(mask.mask);

                while (!killThread.load())
                {
                    while (!killThread.load())
                    {
                        if (XPending(display) != 0)
                        {
                            XEvent event;
                            XNextEvent(display, &event);
                            auto *cookie = reinterpret_cast<XGenericEventCookie *>(&event.xcookie);

                            if ((XGetEventData(display, cookie) != 0) && cookie->type == GenericEvent &&
                                cookie->extension == xiOpCode &&
                                (cookie->evtype == XI_RawKeyRelease || cookie->evtype == XI_RawKeyPress))
                            {
                                auto *ev = reinterpret_cast<XIRawEvent *>(cookie->data);
                                auto key = ev->detail;

                                internal::onKeyEvent(key, cookie->evtype == XI_RawKeyPress);
                            }
                        }
                        else
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    }
                }
            }
        } // namespace internal

        inline void setup()
        {
            internal::keyListener = std::thread(internal::hook);
        }
        inline void stop()
        {
            internal::killThread.store(true);
            internal::keyListener.join();
        }

        inline std::string getKeyName(const int key)
        {
            KeySym s = XkbKeycodeToKeysym(internal::display, key, 0, 0);
            if (NoSymbol == s)
            {
                return "Unknown";
            }

            char *str = XKeysymToString(s);
            if (str == nullptr)
            {
                return "Unknown";
            }

            return str;
        }
    } // namespace Hooks
} // namespace Soundux
#endif