#include "tcplistener.hpp"

#ifndef _WIN32
#define closesocket close
#define SD_BOTH SHUT_RDWR
#endif

namespace Soundux
{
    namespace Network
    {
        bool TCPListener::listen(uint16_t port, int maxConnections)
        {
            if ((m_ServerFD = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                return false;

            struct sockaddr_in address;
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port);

            if (::bind(m_ServerFD, (struct sockaddr *)&address, sizeof(address)) < 0)
                return false;

            if (::listen(m_ServerFD, maxConnections) < 0)
                return false;

            m_Port = port;
            m_MaxConnections = maxConnections;

            return true;
        }

        bool TCPListener::accept(TCPSocket &newSocket)
        {
            int addrlen = sizeof(newSocket.m_RemoteAddr);
            if ((newSocket.m_SocketFD =
                     ::accept(m_ServerFD, (struct sockaddr *)&newSocket.m_RemoteAddr, (socklen_t *)&addrlen)) < 0)
                return false;
            newSocket.m_Connected = true;
            return true;
        }

        void TCPListener::destroy()
        {
            ::closesocket(m_ServerFD);
        }

        bool TCPListener::close()
        {
            if (::shutdown(m_ServerFD, SD_BOTH) == 0)
                return true;
            return false;
        }

        bool TCPListener::setBlocking(bool blocking)
        {
            return Internal::SetBlocking(m_ServerFD, blocking);
        }
    } // namespace Network
} // namespace Soundux