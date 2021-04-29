#include "packethandler.hpp"
#include <core/global/globals.hpp>

namespace Soundux
{
    namespace Objects
    {
        void PacketHandler::HandlePacket(const Network::Protocol::PlaySoundPacket &packet)
        {
            auto pSound = Globals::gGui->playSound(packet.getSoundId());
            if (pSound)
            {
                Globals::gGui->onSoundPlayed(*pSound);
            }
        }
        void PacketHandler::HandlePacket(const Network::Protocol::StopSoundsPacket &)
        {
            if (!Globals::gAudio.getPlayingSounds().empty())
            {
                Globals::gGui->stopSounds();
            }
        }
    } // namespace Objects
} // namespace Soundux
