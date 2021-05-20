#pragma once

#include <cstdint>

namespace Soundux
{
    namespace Objects
    {
        class SounduxClient
        {
          public:
            void playSound(std::uint32_t soundID);
            void stopSounds();
            void hideWindow();
            void showWindow();
        };
    } // namespace Objects
} // namespace Soundux
