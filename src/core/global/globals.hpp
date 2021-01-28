#pragma once
#include "../../helper/audio/audio.hpp"
#include "../../helper/events/eventhandler.hpp"
#include "../../ui/ui.hpp"
#include "../config/config.hpp"
#include "objects.hpp"

#include <memory>

namespace Soundux
{
    namespace Globals
    {
        inline Objects::Data gData;
        inline Objects::Audio gAudio;
        inline Objects::Config gConfig;
        inline Objects::Settings gSettings;
        inline Objects::EventHandler gEvents;
        inline std::shared_ptr<Objects::Window> gGui;

        /* Allows for fast & easy sound access, is populated on start up */
        inline std::map<std::uint32_t, std::reference_wrapper<Objects::Sound>> gSounds;
    } // namespace Globals
} // namespace Soundux