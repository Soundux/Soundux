#pragma once
#include "tcpsocket.hpp"

namespace Soundux
{
    namespace Network
    {
        class TCPListener
        {
            int m_ServerFD;
            uint16_t m_Port;
            int m_MaxConnections;

          public:
            TCPListener() : m_ServerFD(-1) {}

            bool listen(uint16_t port, int maxConnections);
            bool accept(TCPSocket &newSocket);
            bool close();
            void destroy();
            bool setBlocking(bool blocking);

            uint16_t getListeningPort() const
            {
                return m_Port;
            }
            int getMaximumConnections() const
            {
                return m_MaxConnections;
            }
        };
    } // namespace Network
} // namespace Soundux
