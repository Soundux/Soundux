#include "youtube-dl.hpp"
#include <core/global/globals.hpp>
#include <fancy.hpp>
#include <helper/misc/misc.hpp>
#include <optional>

namespace Soundux::Objects
{
    const std::regex YoutubeDl::urlRegex(
        R"(https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&\/\/=]*))");

    void YoutubeDl::setup()
    {
        TinyProcessLib::Process ytdlVersion(
            "youtube-dl --version", "", []([[maybe_unused]] const char *message, [[maybe_unused]] std::size_t size) {},
            []([[maybe_unused]] const char *message, [[maybe_unused]] std::size_t size) {});
        TinyProcessLib::Process ffmpegVersion(
            "ffmpeg -version", "", []([[maybe_unused]] const char *message, [[maybe_unused]] std::size_t size) {},
            []([[maybe_unused]] const char *message, [[maybe_unused]] std::size_t size) {});

        isAvailable = ytdlVersion.get_exit_status() == 0 && ffmpegVersion.get_exit_status() == 0;
        if (!isAvailable)
        {
            Fancy::fancy.logTime().warning() << "youtube-dl or ffmpeg is not available!" << std::endl;
        }
    }
    std::optional<nlohmann::json> YoutubeDl::getInfo(const std::string &url) const
    {
        if (!isAvailable)
        {
            return std::nullopt;
        }

        if (!std::regex_match(url, urlRegex))
        {
            Fancy::fancy.logTime().warning() << "Bad url " >> url << std::endl;
            return std::nullopt;
        }

        auto [result, success] = Helpers::getResultCompact("youtube-dl -i -j \"" + url + "\"");
        if (success)
        {
            auto json = nlohmann::json::parse(result, nullptr, false);
            if (json.is_discarded())
            {
                Fancy::fancy.logTime().warning() << "Failed to parse youtube-dl information" << std::endl;
                Globals::gGui->onError(Enums::ErrorCode::YtdlInvalidJson);
                return std::nullopt;
            }

            nlohmann::json j;
            if (json.find("thumbnails") != json.end())
            {
                j["thumbnails"] = json.at("thumbnails");
            }
            if (json.find("title") != json.end())
            {
                j["title"] = json.at("title");
            }
            if (json.find("uploader") != json.end())
            {
                j["uploader"] = json.at("uploader");
            }

            return j;
        }

        Fancy::fancy.logTime().warning() << "Failed to get info from youtube-dl" << std::endl;
        Globals::gGui->onError(Enums::ErrorCode::YtdlInformationUnknown);
        return std::nullopt;
    }
    bool YoutubeDl::download(const std::string &url)
    {
        if (!isAvailable)
        {
            Globals::gGui->onError(Enums::ErrorCode::YtdlNotFound);
            return false;
        }

        if (currentDownload)
        {
            killDownload();
        }

        if (!std::regex_match(url, urlRegex))
        {
            Fancy::fancy.logTime().warning() << "Bad url " >> url << std::endl;
            Globals::gGui->onError(Enums::ErrorCode::YtdlInvalidUrl);
            return false;
        }

        auto currentTab = Globals::gData.getTab(Globals::gSettings.selectedTab);

        if (currentTab)
        {
            if (currentDownload)
            {
                currentDownload->kill();
                currentDownload.reset();
            }

            currentDownload.emplace("youtube-dl --extract-audio --audio-format mp3 --no-mtime \"" + url + "\" -o \"" +
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
            auto rtn = currentDownload->get_exit_status() == 0;
            currentDownload.reset();
            return rtn;
        }

        Globals::gGui->onError(Enums::ErrorCode::TabDoesNotExist);
        return false;
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
    }
    bool YoutubeDl::available() const
    {
        return isAvailable;
    }
} // namespace Soundux::Objects