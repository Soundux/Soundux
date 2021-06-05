#pragma once
#include <core/enums/enums.hpp>
#include <core/hotkeys/keys.hpp>
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

            std::vector<Key> pushToTalkKeys;
            std::vector<Key> stopHotkey;

            Key remoteVolumeKnob;
            Key localVolumeKnob;

            std::vector<std::string> outputs;
            std::uint32_t selectedTab = 0;

            bool syncVolumes = false;
            int remoteVolume = 100;
            int localVolume = 50;

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