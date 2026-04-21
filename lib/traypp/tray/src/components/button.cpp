#include <components/button.hpp>
#include <utility>

Tray::Button::Button(std::string text, std::function<void()> callback)
    : TrayEntry(std::move(text)), callback(std::move(callback))
{
}

void Tray::Button::clicked()
{
    if (callback)
    {
        callback();
    }
}

void Tray::Button::setCallback(std::function<void()> newCallback)
{
    callback = std::move(newCallback);
}