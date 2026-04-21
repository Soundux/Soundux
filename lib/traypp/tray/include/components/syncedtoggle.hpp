#pragma once
#include <core/entry.hpp>
#include <functional>

namespace Tray
{
    class SyncedToggle : public TrayEntry
    {
        bool &toggled;
        std::function<void(bool &)> callback;

      public:
        ~SyncedToggle() override = default;
        SyncedToggle(
            std::string text, bool &state, std::function<void(bool &)> callback = [](bool & /**/) {});

        void onToggled();
        bool isToggled() const;
    };
} // namespace Tray