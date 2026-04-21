#include <components/imagebutton.hpp>
#include <core/traybase.hpp>
#include <utility>

Tray::ImageButton::ImageButton(std::string text, Image image, std::function<void()> callback)
    : Button(std::move(text), std::move(callback)), image(image)
{
}

void Tray::ImageButton::setImage(Tray::Image newImage)
{
    image = newImage;
    if (parent)
    {
        parent->update();
    }
}

Tray::Image Tray::ImageButton::getImage()
{
    return image;
}