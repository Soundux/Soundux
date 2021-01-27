#include "config.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
namespace Soundux::Objects
{
    const std::string Config::path = []() -> std::string {
#if defined(__linux__)
        const auto *configPath = std::getenv("XDG_CONFIG_HOME"); // NOLINT
        if (configPath)
        {
            return std::string(configPath) + "/Soundux/config.json";
        }
        return std::string(std::getenv("HOME")) + "/.config/Soundux/config.json"; // NOLINT
#elif defined(_WIN32)
        char *buffer;
        std::size_t size;
        _dupenv_s(&buffer, &size, "APPDATA");
        auto rtn = std::string(buffer) + "\\Soundux\\config.json";
        free(buffer);

        return rtn;
#endif
    }();

    void Config::save()
    {
        try
        {
            if (!std::filesystem::exists(path))
            {
                std::filesystem::create_directories(path.substr(0, path.find_last_of('/')));
            }

            std::ofstream configFile(path);
            // TODO(Curve): Write config
            configFile.close();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to write config: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Failed to write config" << std::endl;
        }
    }
    void Config::load()
    {
        try
        {
            if (!std::filesystem::exists(path))
            {
                std::cerr << "Config not found" << std::endl;
                return;
            }

            std::fstream configFile(path);
            // TODO(curve): Read / Parse config file
            configFile.close();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to read config: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Failed to read config" << std::endl;
        }
    }
} // namespace Soundux::Objects