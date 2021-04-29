#pragma once

#include <helper/protocol/packets.hpp>

namespace Soundux
{
    namespace Objects
    {
        class PacketHandler
        {
          public:
            void handlePacket(const Network::Protocol::PlaySoundPacket &);
            void handlePacket(const Network::Protocol::StopSoundsPacket &);
        };
    } // namespace Objects
} // namespace Soundux
