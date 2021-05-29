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
            Enums::BackendType audioBackend = Enums::BackendType::PulseAudio;
            Enums::ViewMode viewMode = Enums::ViewMode::List;
            Enums::Theme theme = Enums::Theme::System;

            std::vector<int> pushToTalkKeys;
            std::vector<int> stopHotkey;

            std::vector<std::string> outputs;
            std::uint32_t selectedTab = 0;

            int remoteVolume = 100;
            int localVolume = 50;
            bool syncVolumes = false;

            bool allowMultipleOutputs = false;
            bool useAsDefaultDevice = false;
            bool muteDuringPlayback = false;
            bool allowOverlapping = true;
            bool minimizeToTray = false;
            bool tabHotkeysOnly = false;
            bool deleteToTrash = true;
        };
    } // namespace Objects
} // namespace Soundux