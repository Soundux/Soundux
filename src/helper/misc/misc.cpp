#include "misc.hpp"
#include <chrono>
#include <exception>
#include <fancy.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <process.hpp>
#include <regex>
#include <system_error>

#if defined(_WIN32)
#include <Windows.h>
#include <shellapi.h>
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
    bool deleteFile(const std::string &path, bool trash)
    {
        if (!trash)
        {
            std::error_code ec;
            std::filesystem::remove(path, ec);

            if (ec)
            {
                Fancy::fancy.logTime().failure() << "Failed to delete file " << path << " error: " << ec.message()
                                                 << "(" << ec.value() << ")" << std::endl;
                return false;
            }
            return true;
        }
#if defined(__linux__)
        std::string home = std::getenv("HOME"); // NOLINT
        if (std::filesystem::exists(home + "/.local/share/Trash/") &&
            std::filesystem::exists(home + "/.local/share/Trash/files") &&
            std::filesystem::exists(home + "/.local/share/Trash/info"))
        {
            auto trashFolder = std::filesystem::canonical(home + "/.local/share/Trash");

            auto filePath = std::filesystem::canonical(path);
            auto trashFileName = filePath.filename().u8string() +
                                 std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            std::error_code ec;
            std::filesystem::rename(filePath, trashFolder / "files" / trashFileName, ec);

            if (ec)
            {
                Fancy::fancy.logTime().failure() << "Failed to move file to trash" << std::endl;
                return false;
            }
            try
            {
                auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                auto tm = *std::gmtime(&time_t); // NOLINT

                std::stringstream ss;
                ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");

                std::ofstream stream(trashFolder / "info" / (trashFileName + ".trashinfo"));
                stream << "[Trash Info]" << std::endl
                       << "Path=" << filePath.u8string() << std::endl
                       << "DeletionDate=" << ss.str() << std::endl;
                stream.close();
            }
            catch (const std::exception &e)
            {
                Fancy::fancy.logTime().failure() << "Failed to create .trashinfo file: " << e.what() << std::endl;
                return false;
            }

            return true;
        }

        Fancy::fancy.logTime().warning() << "Trash folder not found!" << std::endl;
        return false;
#else
        auto filePath = std::filesystem::canonical(widen(path));

        SHFILEOPSTRUCTW operation{};
        operation.pTo = nullptr;
        operation.hwnd = nullptr;
        operation.wFunc = FO_DELETE;

        auto temp = (filePath.wstring() + L'\0');
        operation.pFrom = temp.c_str();

        operation.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOF_NOCONFIRMATION | FOF_SILENT;

        auto res = SHFileOperationW(&operation);
        if (res != 0)
        {
            Fancy::fancy.logTime().failure() << "Failed to move file to trash (" << res << ")" << std::endl;
            return false;
        }

        return true;
#endif
    }
} // namespace Soundux::Helpers