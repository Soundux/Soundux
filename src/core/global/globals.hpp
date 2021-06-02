#pragma once
#include <helper/audio/audio.hpp>
#if defined(__linux__)
#include <helper/audio/linux/backend.hpp>
#elif defined(_WIN32)
#include <helper/audio/windows/winsound.hpp>
#endif
#include <core/config/config.hpp>
#include <core/hotkeys/hotkeys.hpp>
#include <core/objects/data.hpp>
#include <core/objects/objects.hpp>
#include <core/objects/settings.hpp>
#include <helper/icons/icons.hpp>
#include <helper/queue/queue.hpp>
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
        inline std::shared_ptr<Objects::IconFetcher> gIcons;
        inline std::shared_ptr<Objects::AudioBackend> gAudioBackend;
#elif defined(_WIN32)
        inline std::shared_ptr<Objects::WinSound> gWinSound;
#endif
        inline Objects::Queue gQueue;
        inline Objects::Config gConfig;
        inline Objects::YoutubeDl gYtdl;
        inline Objects::Hotkeys gHotKeys;
        inline Objects::Settings gSettings;
        inline std::unique_ptr<Objects::Window> gGui;

        /* Allows for fast & easy sound access, is populated on start up */
        inline sxl::var_guard<std::map<std::uint32_t, std::reference_wrapper<Objects::Sound>>> gSounds;
        inline sxl::var_guard<std::map<std::uint32_t, std::reference_wrapper<Objects::Sound>>> gFavorites;
    } // namespace Globals
} // namespace Soundux