#if defined(__linux__)
#include "icons.hpp"
#include "../base64/base64.hpp"
#include "../misc/misc.hpp"
#include <fancy.hpp>
#include <optional>

namespace Soundux::Objects
{
    void IconFetcher::setup()
    {
        gdk_init(nullptr, nullptr);
        screen = wnck_screen_get_default();
        if (!screen)
        {
            Fancy::fancy.logTime().warning() << "Failed to get default screen!" << std::endl;
        }
    }
    std::optional<std::string> IconFetcher::getIcon(int pid, bool recursive)
    {
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
        if (cache.find(pid) != cache.end())
        {
            return cache.at(pid);
        }

        wnck_screen_force_update(screen);
        auto *windows = wnck_screen_get_windows(screen);

        for (auto *item = windows; item != nullptr; item = item->next)
        {
            auto *window = reinterpret_cast<WnckWindow *>(item->data);
            auto _pid = wnck_window_get_pid(window);

            if (pid == _pid)
            {
                auto *icon = wnck_window_get_icon(window);

                gsize size = 2048;
                auto *iconBuff = new gchar[2048];

                GError *error = nullptr;
                if (gdk_pixbuf_save_to_buffer(icon, &iconBuff, &size, "png", &error, NULL) != TRUE)
                {
                    Fancy::fancy.logTime().warning() << "Failed to save icon to buffer, error: " << error->message
                                                     << "(" << error->code << ")" << std::endl;
                    return std::nullopt;
                }

                auto base64 = base64_encode(reinterpret_cast<const unsigned char *>(iconBuff), 2048, false);
                delete[] iconBuff;

                if (cache.find(pid) == cache.end())
                {
                    cache.insert({pid, base64});
                }

                return base64;
            }
        }

        Fancy::fancy.logTime().warning() << "Could not find proccess with id " >> pid << std::endl;
        return std::nullopt;
    }
} // namespace Soundux::Objects
#endif