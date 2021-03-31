#if defined(_WIN32)
#include "../../misc/misc.hpp"
#include "../systeminfo.hpp"
#include <Windows.h>
#include <fancy.hpp>
#include <regex>

std::string SystemInfo::getSystemInfo()
{
    std::string result;

    if (Soundux::Helpers::exec("cmd /c ver", result))
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