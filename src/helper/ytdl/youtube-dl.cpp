#include "youtube-dl.hpp"
#include "../../core/global/globals.hpp"
#include "../misc/misc.hpp"
#include <fancy.hpp>
#include <regex>

namespace Soundux::Objects
{
    using Helpers::exec;

    void YoutubeDl::setup()
    {
        // TODO(curve): Download ytdl on windows if it doesn't exist
        // TODO(curve): Warning on Linux if youtube-dl doesn't exist
    }
    std::optional<nlohmann::json> YoutubeDl::getInfo(const std::string &url)
    {
        std::smatch match;
        static const std::regex escapeRegex(R"rgx(("|'|\\))rgx");

        if (std::regex_search(url, match, escapeRegex))
        {
            Fancy::fancy.logTime().warning() << "Url " >> url << " contained illegal characters" << std::endl;
            return std::nullopt;
        }

        std::string result;
        if (exec("youtube-dl -i -j \"" + url + "\"", result))
        {
            auto json = nlohmann::json::parse(result, nullptr, false);
            if (json.is_discarded())
            {
                Fancy::fancy.logTime().warning() << "Failed to parse youtube-dl information" << std::endl;
                return std::nullopt;
            }

            nlohmann::json j;
            if (json.find("thumbnails") != json.end() && json.find("title") != json.end() &&
                json.find("uploader") != json.end())
            {
                j["title"] = json.at("title");
                j["uploader"] = json.at("uploader");
                j["thumbnails"] = json.at("thumbnails");

                return j;
            }
            Fancy::fancy.logTime().warning()
                << "Failed to get required information from youtube-dl output" << std::endl;
            return std::nullopt;
        }

        Fancy::fancy.logTime().warning() << "Failed to get info from youtube-dl";
        return std::nullopt;
    }
    void YoutubeDl::download(const std::string &url)
    {
        killDownload();

        std::smatch match;
        static const std::regex escapeRegex(R"rgx(("|'|\\))rgx");

        if (std::regex_search(url, match, escapeRegex))
        {
            Fancy::fancy.logTime().warning() << "Url " >> url << " contained illegal characters" << std::endl;
            return;
        }

        auto currentTab = Globals::gData.getTab(Globals::gSettings.selectedTab);

        if (currentTab)
        {
            if (currentDownload)
            {
                currentDownload->kill();
                currentDownload.reset();
            }

            currentDownload.emplace("youtube-dl --extract-audio --audio-format mp3 \"" + url + "\" -o \"" +
                                        currentTab->path + "/%(title)s.%(ext)s" + "\"",
                                    "", [](const char *rawData, std::size_t dataLen) {
                                        std::string data(rawData, dataLen);
                                        static const std::regex progressRegex(R"(([0-9.,]+)%.*(ETA (.+)))");

                                        std::smatch match;
                                        if (std::regex_search(data, match, progressRegex))
                                        {
                                            if (match[1].matched && match[3].matched)
                                            {
                                                Globals::gGui->onDownloadProgressed(std::stof(match[1]), match[3]);
                                            }
                                        }
                                    });
            Fancy::fancy.logTime().success() << "Started download of " >> url << std::endl;
        }
    }
    void YoutubeDl::killDownload()
    {
        if (currentDownload)
        {
            int status = -1;
            while (!currentDownload->try_get_exit_status(status))
            {
                currentDownload->kill();
            }
            currentDownload.reset();
            Fancy::fancy.logTime().success() << "Killing download, process exited with " >> status << std::endl;
        }
        else
        {
            Fancy::fancy.warning() << "Not currently downloading!" << std::endl;
        }
    }
} // namespace Soundux::Objects