#include "misc.hpp"
#include <fancy.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <process.hpp>
#include <regex>

#if defined(_WIN32)
#include <Windows.h>
#include <stringapiset.h>
#endif

namespace Soundux::Helpers
{
    std::vector<std::string> splitByNewLine(const std::string &str)
    {
        std::vector<std::string> result;
        std::stringstream ss(str);
        for (std::string line; std::getline(ss, line, '\n');)
        {
            result.emplace_back(line);
        }
        return result;
    }
    bool exec(const std::string &command, std::string &result)
    {
        result.clear();
        TinyProcessLib::Process process(
            command, "", [&](const char *data, std::size_t dataLen) { result += std::string(data, dataLen); });

        return process.get_exit_status() == 0;
    }
#if defined(_WIN32)
    std::wstring widen(const std::string &s)
    {
        int wsz = MultiByteToWideChar(65001, 0, s.c_str(), -1, nullptr, 0);
        if (!wsz)
            return std::wstring();

        std::wstring out(wsz, 0);
        MultiByteToWideChar(65001, 0, s.c_str(), -1, &out[0], wsz);
        out.resize(wsz - 1);
        return out;
    }
#endif
#if defined(__linux__)
    std::optional<int> getPpid(int pid)
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
#endif
} // namespace Soundux::Helpers