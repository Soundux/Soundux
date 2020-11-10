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

        struct Song
        {
            std::string name;
            std::string path;
            std::vector<int> hotKeys;
        };
        struct Tab
        {
            std::string title;
            std::string folder;
            std::vector<Song> songs;

            bool operator==(const Tab &other)
            {
                return other.folder == folder && other.title == title;
            }
        };
        struct Config
        {
            std::vector<Tab> tabs;
            unsigned int currentOutputApplication;
            unsigned int currentTab;
            bool tabHotkeysOnly;
            bool darkTheme;
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

        inline void to_json(json &j, const Song &song)
        {
            j = json{{"name", song.name}, {"path", song.path}, {"hotKeys", song.hotKeys}};
        }
        inline void from_json(const json &j, Song &song)
        {
            j.at("name").get_to(song.name);
            j.at("path").get_to(song.path);
            j.at("hotKeys").get_to(song.hotKeys);
        }

        inline void to_json(json &j, const Tab &tab)
        {
            j = json{{"title", tab.title}, {"folder", tab.folder}, {"songs", tab.songs}};
        }
        inline void from_json(const json &j, Tab &tab)
        {
            j.at("title").get_to(tab.title);
            j.at("songs").get_to(tab.songs);
            j.at("folder").get_to(tab.folder);
        }

        inline void to_json(json &j, const Config &config)
        {
            j = json{{"tabs", config.tabs},
                     {"darkTheme", config.darkTheme},
                     {"currentTab", config.currentTab},
                     {"tabHotkeysOnly", config.tabHotkeysOnly}};
        }
        inline void from_json(const json &j, Config &config)
        {
            j.at("tabs").get_to(config.tabs);
            j.at("darkTheme").get_to(config.darkTheme);
            j.at("currentTab").get_to(config.currentTab);
            j.at("tabHotkeysOnly").get_to(config.tabHotkeysOnly);
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