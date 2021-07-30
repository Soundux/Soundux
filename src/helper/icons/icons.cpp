#if defined(__linux__)
#include "icons.hpp"
#include <dlfcn.h>
#include <fancy.hpp>
#include <filesystem>
#include <fstream>
#include <helper/base64/base64.hpp>
#include <optional>
#include <regex>

namespace Soundux::Objects
{
    bool IconFetcher::setup()
    {
        if (!LibWnck::setup())
        {
            Fancy::fancy.logTime().message() << "LibWnck was not found - Icon support is not available" << std::endl;
            return false;
        }

        gdk_init(nullptr, nullptr);
        screen = LibWnck::getDefaultScreen();

        if (!screen)
        {
            Fancy::fancy.logTime().warning() << "Failed to get default screen!" << std::endl;
            return false;
        }

        return true;
    }
    std::shared_ptr<IconFetcher> IconFetcher::createInstance()
    {
        auto instance = std::shared_ptr<IconFetcher>(new IconFetcher()); // NOLINT

        if (instance->setup())
        {
            return instance;
        }

        Fancy::fancy.logTime().failure() << "Could not create IconFetcher instance" << std::endl;
        return nullptr;
    }
    std::optional<int> IconFetcher::getPpid(int pid)
    {
        std::filesystem::path path("/proc/" + std::to_string(pid));
        if (std::filesystem::exists(path))
        {
            auto statusFile = path / "status";
            if (std::filesystem::exists(statusFile) && std::filesystem::is_regular_file(statusFile))
            {
                static const std::regex pidRegex(R"(PPid:(\ +|\t)(\d+))");
                std::ifstream statusStream(statusFile);

                std::string line;
                std::smatch match;
                while (std::getline(statusStream, line))
                {
                    if (std::regex_search(line, match, pidRegex))
                    {
                        if (match[2].matched)
                        {
                            return std::stoi(match[2]);
                        }
                    }
                }

                Fancy::fancy.logTime().warning() << "Failed to find ppid of " >> pid << std::endl;
                return std::nullopt;
            }
        }

        Fancy::fancy.logTime().warning() << "Failed to find ppid of " >> pid << ", process does not exist" << std::endl;
        return std::nullopt;
    }
    std::optional<std::string> IconFetcher::getIcon(int pid, bool recursive)
    {
        if (cache.find(pid) != cache.end())
        {
            return cache.at(pid);
        }

        LibWnck::forceUpdate(screen);
        auto *windows = LibWnck::getScreenWindows(screen);

        for (auto *item = windows; item != nullptr; item = item->next)
        {
            auto *window = reinterpret_cast<LibWnck::Window *>(item->data);
            auto _pid = LibWnck::getWindowPID(window);

            if (pid == _pid)
            {
                auto *icon = LibWnck::getWindowIcon(window);

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
            auto parentProcess = getPpid(pid);
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