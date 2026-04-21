#include <components/toggle.hpp>

Tray::Toggle::Toggle(std::string text, bool state, std::function<void(bool)> callback)
    : TrayEntry(std::move(text)), toggled(state), callback(std::move(callback))
{
}

bool Tray::Toggle::isToggled() const
{
    return toggled;
}

void Tray::Toggle::onToggled()
{
    toggled = !toggled;
    callback(toggled);
}