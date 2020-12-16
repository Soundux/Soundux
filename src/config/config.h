#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <json.hpp>
#include <iostream>
#include <streambuf>
#include <exception>
#include <filesystem>

namespace Soundux
{
    namespace Config
    {
        using namespace nlohmann;

        struct Sound
        {
            std::string name;
            std::string path;
            std::vector<int> hotKeys;
        };
        struct Tab
        {
            std::string title;
            std::string folder;
            std::vector<Sound> sounds;

            bool operator==(const Tab &other)
            {
                return other.folder == folder && other.title == title;
            }
        };
        struct Config
        {
            std::vector<Tab> tabs;

            std::map<std::string, float> volumes;
            unsigned int currentOutputApplication;
            std::vector<int> stopHotKey;
            unsigned int currentTab;

            bool allowOverlapping = false;
            bool darkTheme = true;
            bool tabHotkeysOnly;

            int width = 995, height = 550;
        };

        inline Config gConfig;

#ifdef __linux__
        inline std::string configPath = std::string(getenv("HOME")) + "/.config/Soundux/config.json";
#else
#if _WIN32
        inline std::string configPath = std::string(getenv("APPDATA")) + "/Soundux/config.json";
#else
// is mac
#endif
#endif

        inline void to_json(json &j, const Sound &sound)
        {
            j = json{{"name", sound.name}, {"path", sound.path}, {"hotKeys", sound.hotKeys}};
        }
        inline void from_json(const json &j, Sound &sound)
        {
            j.at("name").get_to(sound.name);
            j.at("path").get_to(sound.path);
            j.at("hotKeys").get_to(sound.hotKeys);
        }

        inline void to_json(json &j, const Tab &tab)
        {
            j = json{{"title", tab.title}, {"folder", tab.folder}, {"sounds", tab.sounds}};
        }
        inline void from_json(const json &j, Tab &tab)
        {
            j.at("title").get_to(tab.title);
            j.at("sounds").get_to(tab.sounds);
            j.at("folder").get_to(tab.folder);
        }

        inline void to_json(json &j, const Config &config)
        {
            j = json{
                {"tabs", config.tabs},
                {"darkTheme", config.darkTheme},
                {"allowOverlapping", config.allowOverlapping},
                {"currentTab", config.currentTab},
                {"tabHotkeysOnly", config.tabHotkeysOnly},
                {"stopHotKey", config.stopHotKey},
                {"volumes", config.volumes},
                {"width", config.width},
                {"height", config.height},
            };
        }
        inline void from_json(const json &j, Config &config)
        {
            j.at("tabs").get_to(config.tabs);
            j.at("darkTheme").get_to(config.darkTheme);
            j.at("currentTab").get_to(config.currentTab);
            j.at("stopHotKey").get_to(config.stopHotKey);
            j.at("tabHotkeysOnly").get_to(config.tabHotkeysOnly);

            if (j.contains("volumes"))
                j.at("volumes").get_to(config.volumes);
            if (j.contains("width"))
                j.at("width").get_to(config.width);
            if (j.contains("height"))
                j.at("height").get_to(config.height);
            if (j.contains("allowOverlapping"))
                j.at("allowOverlapping").get_to(config.allowOverlapping);
        }

        inline void loadConfig()
        {
            if (!std::filesystem::exists(configPath))
            {
                std::cerr << "Config file not found"
                          << std::endl; // This is not a fatal error, maybe the config file just isn't created yet
                return;
            }
            if (!std::filesystem::is_regular_file(configPath))
            {
                std::cerr << "Config should be a file" << std::endl;
                return;
            }
            if (std::filesystem::is_empty(configPath))
            {
                return;
            }

            std::fstream fileStream(configPath);
            json parsed;

            try
            {
                parsed = json::parse(fileStream);
            }
            catch (json::parse_error &err)
            {
                std::cerr << err.what() << std::endl;
                fileStream.close();
                return;
            }

            fileStream.close();

            try
            {
                gConfig = parsed.get<Config>();
            }
            catch (...)
            {
                std::cerr << "Faied to read config" << std::endl;
            }
        }
        inline void saveConfig()
        {
            try
            {
                if (!std::filesystem::exists(configPath))
                {
                    std::filesystem::create_directories(configPath.substr(0, configPath.find_last_of('/')));
                }

                std::ofstream outStream(configPath);
                json configJson = gConfig;

                outStream << configJson.dump() << std::endl;

                outStream.close();
            }
            catch (std::exception &e)
            {
                std::cerr << "Saving config failed: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Error while saving config file" << std::endl;
            }
        }
    } // namespace Config
} // namespace Soundux