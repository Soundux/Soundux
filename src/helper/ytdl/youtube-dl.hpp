#pragma once
#include <json.hpp>
#include <optional>
#include <process.hpp>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class YoutubeDl
        {
            bool isAvailable = false;
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