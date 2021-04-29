#include "packets.hpp"

namespace Soundux
{
    namespace Network
    {
        namespace Protocol
        {
            Buffer &operator<<(Buffer &buffer, const PlaySoundPacket &packet)
            {
                buffer << packet.getPacketLength() << packet.getPacketType() << packet.getSoundId();
                return buffer;
            }

            const Buffer &operator>>(Buffer &buffer, PlaySoundPacket &packet)
            {
                buffer >> packet.m_SoundID;
                return buffer;
            }

            std::uint32_t PlaySoundPacket::getPacketLength() const
            {
                return sizeof(PacketType) + sizeof(m_SoundID);
            }

            Buffer &operator<<(Buffer &buffer, const StopSoundsPacket &packet)
            {
                buffer << packet.getPacketLength() << packet.getPacketType();
                return buffer;
            }

            std::uint32_t StopSoundsPacket::getPacketLength() const
            {
                return sizeof(PacketType);
            }

        } // namespace Protocol
    }     // namespace Network
} // namespace Soundux