#pragma once
#include <optional>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Helpers
    {
#if defined(_WIN32)
        std::wstring widen(const std::string &);
        std::string narrow(const std::wstring &);
#endif
        bool deleteFile(const std::string &, bool = true);

        bool run(const std::string &);
        std::pair<std::string, bool> getResultCompact(const std::string &);
        std::pair<std::vector<std::string>, bool> getResult(const std::string &);

    } // namespace Helpers
} // namespace Soundux