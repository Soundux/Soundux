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
            std::optional<TinyProcessLib::Process> currentDownload;

          public:
            void setup();
            void killDownload();
            void download(const std::string &);
            std::optional<nlohmann::json> getInfo(const std::string &);
        };
    } // namespace Objects
} // namespace Soundux