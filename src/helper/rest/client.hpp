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
        };
    } // namespace Objects
} // namespace Soundux
