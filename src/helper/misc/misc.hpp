#pragma once
#include <string>
#include <vector>

namespace Soundux
{
    namespace Helpers
    {
#if defined(_WIN32)
        std::wstring widen(const std::string &s);
#endif
#if defined(__linux__)
        bool exec(const std::string &command, std::string &result);
#endif
        std::vector<std::string> splitByNewLine(const std::string &str);
    } // namespace Helpers
} // namespace Soundux