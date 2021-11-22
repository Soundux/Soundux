#pragma once
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
#include <guard.hpp>
#include <helper/icons/icons.hpp>
#include <helper/queue/queue.hpp>
#include <helper/ytdl/youtube-dl.hpp>
#include <lock.hpp>
#include <memory>
#include <ui/ui.hpp>

namespace Soundux
{
    namespace Globals
    {
        inline Objects::Data gData;
#if defined(__linux__)
        inline std::shared_ptr<Objects::IconFetcher> gIcons;
        inline std::shared_ptr<Objects::AudioBackend> gAudioBackend;
#elif defined(_WIN32)
        inline std::shared_ptr<Objects::WinSound> gWinSound;
#endif
        inline Objects::Queue gQueue;
        inline Objects::Config gConfig;
        inline Objects::YoutubeDl gYtdl;
        inline Objects::Settings gSettings;
        inline std::unique_ptr<Objects::Window> gGui;
        inline std::shared_ptr<Objects::Hotkeys> gHotKeys;

        inline std::shared_ptr<Instance::Guard> gGuard;

        /* Allows for fast & easy sound access, is populated on start up */
        inline sxl::lock<std::map<std::uint32_t, std::reference_wrapper<Objects::Sound>>> gSounds;
        inline sxl::lock<std::map<std::uint32_t, std::reference_wrapper<Objects::Sound>>> gFavorites;
    } // namespace Globals
} // namespace Soundux