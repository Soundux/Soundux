#include "server.hpp"
#include "tcplistener.hpp"
#include <core/global/globals.hpp>

namespace Soundux
{
    namespace Objects
    {
        void SounduxServer::onReceived(Network::Buffer &buffer)
        {
            using namespace Network::Protocol;

            PacketType packetType;
            buffer >> packetType;

            switch (packetType)
            {
            case PacketType::PlaySoundID: {
                PlaySoundPacket packet;
                buffer >> packet;
                Globals::gHandler.handlePacket((Network::Protocol::PlaySoundPacket &)packet);
                break;
            }

            case PacketType::StopSounds: {
                StopSoundsPacket packet;
                Globals::gHandler.handlePacket((Network::Protocol::StopSoundsPacket &)packet);
                break;
            }

            default:
                break;
            }
        }

        void SounduxServer::init()
        {
            Network::InitialiseNetwork();
            m_ListenThread = std::thread([this]() {
                m_TcpListener.listen(SOUNDUX_PORT, 1);
                m_TcpListener.setBlocking(true);
                while (!m_ShouldStop)
                {
                    Network::TCPSocket newSocket;
                    if (!m_TcpListener.accept(newSocket))
                    {
                        continue;
                    }
                    newSocket.setBlocking(true);
                    Network::Buffer data;
                    std::size_t received = newSocket.receive(data, 4096);
                    std::size_t totalReceived = received;
                    if (received > 0)
                    {
                        uint32_t packetLength;
                        data >> packetLength;
                        while (totalReceived < data.getBufferSize())
                        {
                            totalReceived += newSocket.receive(data, 4096);
                        }
                        // skipping the length info
                        data.setReadOffset(sizeof(uint32_t));
                        onReceived(data);
                    }
                    newSocket.disconnect();
                }
            });
        }
        void SounduxServer::destroy()
        {
            m_ShouldStop = true;
            m_TcpListener.close();
            m_ListenThread.join();
            m_TcpListener.destroy();
            Network::ReleaseNetwork();
        }
    } // namespace Objects
} // namespace Soundux
