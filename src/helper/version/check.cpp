#include "check.hpp"
#include <fancy.hpp>
#include <json.hpp>
#include <regex>

httplib::Client VersionCheck::client("https://api.github.com");

void VersionCheck::setup()
{
    client.enable_server_certificate_verification(false);
}

bool VersionCheck::isLatest()
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
                        if (match[1] != SOUNDUX_VERSION)
                        {
                            Fancy::fancy.logTime().warning() << "Current version is " << SOUNDUX_VERSION
                                                             << " latest version is " << match[1] << std::endl;
                            return false;
                        }
                        Fancy::fancy.logTime().success() << "You are using the latest version of soundux" << std::endl;
                        return true;
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
    return true;
}