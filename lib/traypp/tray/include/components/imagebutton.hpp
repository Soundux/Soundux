#pragma once
#include "button.hpp"
#include <core/image.hpp>

namespace Tray
{
    class ImageButton : public Button
    {
        Image image;

      public:
        ~ImageButton() override = default;
        ImageButton(
            std::string text, Image image, std::function<void()> callback = [] {});

        Image getImage();
        void setImage(Image);
    };
} // namespace Tray