/*

    We will probably make use of this: https://github.com/anko/xkbcat
    This will probably fix #12 and #15

*/
#pragma once
#include <atomic>
#include <thread>
#include <iostream>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>

namespace Soundux
{
    namespace Hooks
    {
        namespace internal
        {
            inline std::thread keyListener;
            inline std::atomic<bool> killThread = false;

            inline void hook()
            {
                Display *display = XOpenDisplay(":0");
                if (display == NULL)
                {
                    std::cerr << "Failed to open X11 Display" << std::endl;
                    return;
                }

                int xiOpCode, queryEvent, queryError;
                if (!XQueryExtension(display, "XInputExtension", &xiOpCode, &queryEvent, &queryError))
                {
                    std::cerr << "XInput extension is not aviable" << std::endl;
                    return;
                }

                {
                    int major = 2, minor = 0;
                    int queryResult = XIQueryVersion(display, &major, &minor);
                    if (queryResult == BadRequest)
                    {
                        std::cerr << "XI 2.0 support is required - Current Version: " << major << "." << minor
                                  << std::endl;
                        return;
                    }
                    else if (queryResult != Success)
                    {
                        std::cerr << "Unknown error" << std::endl;
                        return;
                    }
                }

                Window root = DefaultRootWindow(display);
                XIEventMask mask;
                mask.deviceid = XIAllMasterDevices;
                mask.mask_len = XIMaskLen(XI_LASTEVENT);
                mask.mask = (unsigned char *)calloc(mask.mask_len, sizeof(char));

                XISetMask(mask.mask, XI_RawKeyPress);
                XISelectEvents(display, root, &mask, 1);
                XSync(display, false);
                free(mask.mask);

                while (!killThread.load())
                {
                    XEvent event;
                    XGenericEventCookie *cookie = reinterpret_cast<XGenericEventCookie *>(&event.xcookie);
                    XNextEvent(display, &event);

                    if (XGetEventData(display, cookie) && cookie->type == GenericEvent && cookie->extension == xiOpCode)
                    {
                        XIRawEvent *ev = reinterpret_cast<XIRawEvent *>(cookie->data);
                        auto key = ev->detail;

                        KeySym s = XkbKeycodeToKeysym(display, key, 0, 0);
                        if (NoSymbol == s)
                            continue;
                        char *str = XKeysymToString(s);
                        if (NULL == str)
                            continue;

                        std::cout << str << std::endl;
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

    } // namespace Hooks
} // namespace Soundux