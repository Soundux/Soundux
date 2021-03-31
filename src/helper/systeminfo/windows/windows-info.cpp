#if defined(_WIN32)
#include "../../misc/misc.hpp"
#include "../systeminfo.hpp"
#include <Windows.h>
#include <fancy.hpp>

std::string SystemInfo::getSystemInfo()
{
    std::string result;

    if (Soundux::Helpers::exec("cmd /c winver", result))
    {
        if (result.empty())
        {
            result = "winver failed" << std::endl;
        }
    }

    result += "\nSOUNDUX_VERSION: " SOUNDUX_VERSION;
}
#endif