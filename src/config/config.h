#pragma once
#include <string>
#include <vector>
#include <fstream>
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
            std::string hotKey;
        };
        struct Tab
        {
            std::string title;
            std::string folder;
            std::vector<Song> songs;
        };
        struct Config
        {
            std::vector<Tab> tabs;
        };

        inline Config gConfig;
        inline std::string configPath; // Set this depending on OS

        /*These are used, ignore possible warnings*/
        [[maybe_unused]] void to_json(json &j, const Song &song)
        {
            j = json{{"name", song.name}, {"path", song.path}, {"hotKey", song.hotKey}};
        }
        [[maybe_unused]] void from_json(const json &j, Song &song)
        {
            j.at("name").get_to(song.name);
            j.at("path").get_to(song.path);
            j.at("hotKey").get_to(song.hotKey);
        }

        [[maybe_unused]] void to_json(json &j, const Tab &tab)
        {
            j = json{{"title", tab.title}, {"folder", tab.folder}, {"songs", tab.songs}};
        }
        [[maybe_unused]] void from_json(const json &j, Tab &tab)
        {
            j.at("title").get_to(tab.title);
            j.at("songs").get_to(tab.songs);
            j.at("folder").get_to(tab.folder);
        }

        [[maybe_unused]] void to_json(json &j, const Config &config)
        {
            j = json{{"tabs", config.tabs}};
        }
        [[maybe_unused]] void from_json(const json &j, Config &config)
        {
            j.at("tabs").get_to(config.tabs);
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