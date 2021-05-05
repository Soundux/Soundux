#pragma once
#include <optional>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Helpers
    {
#if defined(_WIN32)
        std::wstring widen(const std::string &s);
        std::string narrow(const std::wstring &s);
#endif
#if defined(__linux__)
        std::optional<int> getPpid(int pid);
#endif

        bool deleteFile(const std::string &path, bool trash = true);

        bool run(const std::string &command);
        std::pair<std::string, bool> getResultCompact(const std::string &command);
        std::pair<std::vector<std::string>, bool> getResult(const std::string &command);

    } // namespace Helpers
} // namespace Soundux