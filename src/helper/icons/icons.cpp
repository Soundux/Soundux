#if defined(__linux__)
#include "icons.hpp"
#include "../base64/base64.hpp"
#include "../misc/misc.hpp"
#include <dlfcn.h>
#include <fancy.hpp>
#include <optional>

namespace Soundux::Objects
{
    void IconFetcher::setup()
    {
        auto *libWnck = dlopen("libwnck-3.so", RTLD_LAZY);
        if (libWnck)
        {
            isAvailable = true;
            Fancy::fancy.logTime().success() << "LibWnck found - Icon support is enabled" << std::endl;

            Lib::wnckGetDefaultScreen =
                reinterpret_cast<decltype(Lib::wnckGetDefaultScreen)>(dlsym(libWnck, "wnck_screen_get_default"));
            Lib::wnckForceUpdate =
                reinterpret_cast<decltype(Lib::wnckForceUpdate)>(dlsym(libWnck, "wnck_screen_force_update"));
            Lib::wnckGetScreenWindows =
                reinterpret_cast<decltype(Lib::wnckGetScreenWindows)>(dlsym(libWnck, "wnck_screen_get_windows"));
            Lib::wnckGetWindowPID =
                reinterpret_cast<decltype(Lib::wnckGetWindowPID)>(dlsym(libWnck, "wnck_window_get_pid"));
            Lib::wnckGetWindowIcon =
                reinterpret_cast<decltype(Lib::wnckGetWindowIcon)>(dlsym(libWnck, "wnck_window_get_icon"));
        }
        else
        {
            Fancy::fancy.logTime().message() << "LibWnck was not found - Icon support is not available" << std::endl;
            return;
        }

        gdk_init(nullptr, nullptr);
        screen = Lib::wnckGetDefaultScreen();
        if (!screen)
        {
            Fancy::fancy.logTime().warning() << "Failed to get default screen!" << std::endl;
        }
    }
    std::optional<std::string> IconFetcher::getIcon(int pid, bool recursive)
    {
        if (!isAvailable)
        {
            return std::nullopt;
        }

        if (cache.find(pid) != cache.end())
        {
            return cache.at(pid);
        }

        Lib::wnckForceUpdate(screen);
        auto *windows = Lib::wnckGetScreenWindows(screen);

        for (auto *item = windows; item != nullptr; item = item->next)
        {
            auto *window = reinterpret_cast<Lib::WnckWindow *>(item->data);
            auto _pid = Lib::wnckGetWindowPID(window);

            if (pid == _pid)
            {
                auto *icon = Lib::wnckGetWindowIcon(window);

                gsize size = 4096;
                auto *iconBuff = new gchar[size];

                GError *error = nullptr;
                if (gdk_pixbuf_save_to_buffer(icon, &iconBuff, &size, "png", &error, NULL) != TRUE)
                {
                    Fancy::fancy.logTime().warning() << "Failed to save icon to buffer, error: " << error->message
                                                     << "(" << error->code << ")" << std::endl;
                    delete[] iconBuff;
                    return std::nullopt;
                }

                auto base64 = base64_encode(reinterpret_cast<const unsigned char *>(iconBuff), size, false);
                delete[] iconBuff;

                if (cache.find(pid) == cache.end())
                {
                    cache.insert({pid, base64});
                }

                return base64;
            }
        }

        if (recursive)
        {
            auto parentProcess = Helpers::getPpid(pid);
            if (parentProcess)
            {
                auto recursiveResult = getIcon(*parentProcess, false);
                if (recursiveResult)
                {
                    return recursiveResult;
                }
            }
        }

        Fancy::fancy.logTime().warning() << "Could not find proccess with id " >> pid << std::endl;
        return std::nullopt;
    }
} // namespace Soundux::Objects
#endif