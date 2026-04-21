#pragma once
#include <core/entry.hpp>
#include <functional>

namespace Tray
{
    class Button : public TrayEntry
    {
        std::function<void()> callback;

      public:
        ~Button() override = default;
        Button(
            std::string text, std::function<void()> callback = [] {});

        void clicked();
        void setCallback(std::function<void()>);
    };
} // namespace Tray