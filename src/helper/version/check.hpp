#pragma once
#include <httplib.h>
#include <optional>

namespace Soundux
{
    namespace Objects
    {
        struct VersionStatus
        {
            std::string current;
            std::string latest;
            bool outdated;
        };
    } // namespace Objects
} // namespace Soundux

class VersionCheck
{
    static httplib::Client client;

  public:
    static std::optional<Soundux::Objects::VersionStatus> getStatus();
};