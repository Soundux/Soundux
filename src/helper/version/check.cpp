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
            auto latestTag = parsed[0]["name"];
            if (!latestTag.is_null())
            {
                static std::regex lastNumber(R"(.*(\d))");
                bool outdated = false;

                std::smatch match;
                auto latestTagStr = latestTag.get<std::string>();
                if (std::regex_search(latestTagStr, match, lastNumber))
                {
                    auto remoteNumber = std::stoi(match[1]);
                    auto localVersion = std::string(SOUNDUX_VERSION);
                    if (std::regex_search(localVersion, match, lastNumber))
                    {
                        auto localNumber = std::stoi(match[1]);

                        if (remoteNumber > localNumber)
                        {
                            outdated = true;
                        }
                    }
                }

                return Soundux::Objects::VersionStatus{SOUNDUX_VERSION, latestTag, outdated};
            }
            Fancy::fancy.logTime().warning() << "Failed to find latest tag" << std::endl;
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