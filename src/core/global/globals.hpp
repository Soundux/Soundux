#pragma once
#include <helper/audio/audio.hpp>
#if defined(__linux__)
#include <helper/audio/linux/backend.hpp>
#endif
#include "objects.hpp"
#include <core/config/config.hpp>
#include <core/hotkeys/hotkeys.hpp>
#include <helper/icons/icons.hpp>
#include <helper/threads/processing.hpp>
#include <helper/ytdl/youtube-dl.hpp>
#include <memory>
#include <ui/ui.hpp>
#include <var_guard.hpp>

namespace Soundux
{
    namespace Globals
    {
        inline Objects::Data gData;
        inline Objects::Audio gAudio;
#if defined(__linux__)
        inline Objects::IconFetcher gIcons;
        inline std::shared_ptr<Objects::AudioBackend> gAudioBackend;
#endif
        inline Objects::Config gConfig;
        inline Objects::YoutubeDl gYtdl;
        inline Objects::Hotkeys gHotKeys;
        inline Objects::Settings gSettings;
        inline std::unique_ptr<Objects::Window> gGui;
        inline Objects::ProcessingQueue<std::uintptr_t> gQueue;

        /* Allows for fast & easy sound access, is populated on start up */
        inline sxl::var_guard<std::map<std::uint32_t, std::reference_wrapper<Objects::Sound>>> gSounds;
        inline sxl::var_guard<std::map<std::uint32_t, std::reference_wrapper<Objects::Sound>>> gFavorites;
    } // namespace Globals
} // namespace Soundux