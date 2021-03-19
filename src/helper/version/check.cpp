#include "check.hpp"
#include <fancy.hpp>
#include <json.hpp>
#include <optional>
#include <regex>

httplib::Client VersionCheck::client("https://api.github.com");

std::optional<Soundux::Objects::VersionStatus> VersionCheck::getStatus()
{
    auto githubTags = client.Get("/repos/Soundux/Soundux/tags");

    if (githubTags && githubTags->status == 200)
    {
        auto parsed = nlohmann::json::parse(githubTags->body, nullptr, false);

        if (!parsed.is_discarded())
        {
            auto _latestTag = parsed[0]["name"];
            if (!_latestTag.is_null())
            {
                static const std::regex versionRegex(R"(^([0-9. ]+).*$)");
                std::smatch match;

                auto latestTag = _latestTag.get<std::string>();
                if (std::regex_search(latestTag, match, versionRegex))
                {
                    if (match[1].matched)
                    {
                        return Soundux::Objects::VersionStatus{SOUNDUX_VERSION, match[1], match[1] != SOUNDUX_VERSION};
                    }
                    Fancy::fancy.logTime().warning() << "Failed to fetch latest version" << std::endl;
                }
                else
                {
                    Fancy::fancy.logTime().warning() << "Failed to parse latest version" << std::endl;
                }
            }
            else
            {
                Fancy::fancy.logTime().warning() << "Failed to find latest tag" << std::endl;
            }
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Failed to parse github response" << std::endl;
        }
    }
    else
    {
        Fancy::fancy.logTime().warning() << "Request failed!" << std::endl;
    }
    return std::nullopt;
}