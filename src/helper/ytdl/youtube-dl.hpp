#pragma once
#include <json.hpp>
#include <optional>
#include <process.hpp>
#include <regex>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class YoutubeDl
        {
            bool isAvailable = false;
            static const std::regex urlRegex;
            std::optional<TinyProcessLib::Process> currentDownload;

          public:
            void setup();
            void killDownload();
            bool available() const;
            bool download(const std::string &);
            std::optional<nlohmann::json> getInfo(const std::string &) const;
        };
    } // namespace Objects
} // namespace Soundux