#pragma once

#include <helper/protocol/packets.hpp>

namespace Soundux
{
    namespace Objects
    {
        class PacketHandler
        {
          public:
            void HandlePacket(const Network::Protocol::PlaySoundPacket &);
            void HandlePacket(const Network::Protocol::StopSoundsPacket &);
        };
    } // namespace Objects
} // namespace Soundux
