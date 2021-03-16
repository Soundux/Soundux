#if defined(_WIN32)
#include "../systeminfo.hpp"
#include <Windows.h>
#include <fancy.hpp>

std::string SystemInfo::getSystemInfo()
{
    OSVERSIONINFOEX info;
    ZeroMemory(&info, sizeof(OSVERSIONINFOEX));
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    GetVersionEx(&info);

    return ("Windows: " + std::to_string(info.dwMajorVersion) + "." + std::to_string(info.dwMinorVersion) + "\n");
}
#endif