#pragma once
#include <core/enums/enums.hpp>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct Settings
        {
            Enums::SortMode sortMode = Enums::SortMode::ModifiedDate_Descending;
            Enums::BackendType audioBackend = Enums::BackendType::PulseAudio;
            Enums::ViewMode viewMode = Enums::ViewMode::List;
            Enums::Theme theme = Enums::Theme::System;

            std::vector<int> pushToTalkKeys;
            std::vector<int> stopHotkey;

            std::uint32_t selectedTab = 0;
            std::string output;

            float remoteVolume = 1.f;
            float localVolume = 0.5f;
            bool syncVolumes = false;

            bool useAsDefaultDevice = false;
            bool muteDuringPlayback = false;
            bool allowOverlapping = true;
            bool minimizeToTray = false;
            bool tabHotkeysOnly = false;
            bool deleteToTrash = true;
        };
    } // namespace Objects
} // namespace Soundux