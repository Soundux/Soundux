#include <components/syncedtoggle.hpp>

Tray::SyncedToggle::SyncedToggle(std::string text, bool &state, std::function<void(bool &)> callback)
    : TrayEntry(std::move(text)), toggled(state), callback(std::move(callback))
{
}

bool Tray::SyncedToggle::isToggled() const
{
    return toggled;
}

void Tray::SyncedToggle::onToggled()
{
    toggled = !toggled;
    callback(toggled);
}