#if defined(__linux__)
#include "../systeminfo.hpp"
#include <fancy.hpp>
#include <helper/misc/misc.hpp>

std::string SystemInfo::getSystemInfo()
{
    auto [result, success] = Soundux::Helpers::getResultCompact("lsb_release -a");

    if (success)
    {
        if (result.empty())
        {
            Fancy::fancy.logTime().warning() << "lsb_release output was empty!" << std::endl;
            result = "lsb_release failed";
        }
    }
    else
    {
        Fancy::fancy.logTime().warning() << "lsb_release failed!" << std::endl;
    }

    auto *desktopEnv = std::getenv("XDG_CURRENT_DESKTOP"); // NOLINT
    if (desktopEnv != nullptr)
    {
        result += "\nCurrent Desktop: " + std::string(desktopEnv) + "\n";
    }
    else
    {
        Fancy::fancy.logTime().warning() << "XDG_CURRENT_DESKTOP not set" << std::endl;
    }

    auto *sessionType = std::getenv("XDG_SESSION_TYPE"); // NOLINT
    if (sessionType != nullptr)
    {
        result += "\nCurrent Session: " + std::string(sessionType) + "\n";
    }
    else
    {
        Fancy::fancy.logTime().warning() << "XDG_SESSION_TYPE not set" << std::endl;
    }

    return result;
}
#endif