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
                return Soundux::Objects::VersionStatus{SOUNDUX_VERSION, latestTag, latestTag != SOUNDUX_VERSION};
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