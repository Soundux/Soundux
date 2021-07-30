#if defined(_WIN32)
#include "../systeminfo.hpp"
#include <Windows.h>
#include <fancy.hpp>
#include <helper/misc/misc.hpp>
#include <regex>

std::string SystemInfo::getSystemInfo()
{
    auto [result, success] = Soundux::Helpers::getResultCompact("cmd /c ver");
    if (success)
    {
        if (result.empty())
        {
            result = "winver failed\n";
        }
    }

    result.erase(std::remove(result.begin(), result.end(), '\r'));
    return result;
}
#endif