#include <core/traybase.hpp>

Tray::BaseTray::BaseTray(std::string identifier, Icon icon) : icon(std::move(icon)), identifier(std::move(identifier))
{
}

std::vector<std::shared_ptr<Tray::TrayEntry>> Tray::BaseTray::getEntries()
{
    return entries;
}