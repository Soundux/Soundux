#include "systeminfo.hpp"
#include "../../core/global/globals.hpp"
#include "../json/bindings.hpp"

std::string SystemInfo::getSettingsInfo()
{
    auto settingsJson = nlohmann::json(Soundux::Globals::gSettings);

    std::string rtn;
    for (const auto &item : settingsJson.items())
    {
        rtn += item.key() + ": " + item.value().dump() + "\n";
    }

    return rtn;
}

std::string SystemInfo::getSummary()
{
    return getSystemInfo() + "\n" + getSettingsInfo() + "\n" + "SOUNDUX_VERSION: " + SOUNDUX_VERSION;
}