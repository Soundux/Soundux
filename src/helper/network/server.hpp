#pragma once

#include <thread>
#include <helper/network/tcplistener.hpp>

namespace Soundux
{
    namespace Network
    {
        class Buffer;
    } // namespace Network
    namespace Objects
    {
        class SounduxServer
        {
			Network::TCPListener m_TcpListener;
            std::thread m_ListenThread;
            bool m_ShouldStop = false;

          public:
            void init();
			void destroy();

          private:
            void OnReceived(Network::Buffer &);
        };
    } // namespace Objects
} // namespace Soundux
