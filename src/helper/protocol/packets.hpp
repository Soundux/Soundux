#pragma once

#include <cstdint>
#include <helper/network/tcpsocket.hpp>

namespace Soundux
{
    namespace Network
    {
        namespace Protocol
        {

            enum PacketType : std::uint8_t
            {
                PlaySoundID = 0,
                StopSounds
            };

            class Packet
            {
              public:
                virtual PacketType getPacketType() const = 0;
                // the maximum size of a packet should be 4'294'967'296 bytes which is big enough
                virtual std::uint32_t getPacketLength() const = 0;
            };

            class PlaySoundPacket : public Packet
            {
                std::uint32_t m_SoundID;

              public:
                PlaySoundPacket() : m_SoundID(0) {}
                PlaySoundPacket(std::uint32_t soundID) : m_SoundID(soundID) {}
                virtual PacketType getPacketType() const
                {
                    return PacketType::PlaySoundID;
                }
                virtual std::uint32_t getPacketLength() const;
                uint32_t getSoundId() const
                {
                    return m_SoundID;
                }

                friend Buffer &operator<<(Buffer &buffer, const PlaySoundPacket &packet);
                friend const Buffer &operator>>(Buffer &buffer, PlaySoundPacket &packet);
            };

            class StopSoundsPacket : public Packet
            {
              public:
                StopSoundsPacket() {}
                virtual PacketType getPacketType() const
                {
                    return PacketType::StopSounds;
                }
                virtual std::uint32_t getPacketLength() const;

                friend Buffer &operator<<(Buffer &buffer, const StopSoundsPacket &packet);
            };

        } // namespace Protocol
    }     // namespace Network
} // namespace Soundux
