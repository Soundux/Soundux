#pragma once
#include <core/entry.hpp>
#include <functional>

namespace Tray
{
    class Label : public TrayEntry
    {
      public:
        Label(std::string text);
        ~Label() override = default;
    };
} // namespace Tray