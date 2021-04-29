#include "tcpsocket.hpp"
#ifdef _WIN32
#define WOULDBLOCK WSAEWOULDBLOCK
#define MSG_DONTWAIT 0
#else
#define WOULDBLOCK EWOULDBLOCK
#define closesocket close
#endif

namespace Soundux
{
    namespace Network
    {
        bool TCPSocket::connect(const std::string &address, unsigned short port)
        {
            if (m_Connected)
                return true;

            struct addrinfo hints
            {
            }, *result = nullptr;

            if ((m_SocketFD = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                return false;

            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            if (::getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &result) < 0)
                return false;

            addrinfo *ptr = nullptr;
            for (ptr = result; ptr != nullptr; ptr = ptr->ai_next)
            {
                sockaddr_in *sockaddr = reinterpret_cast<sockaddr_in *>(ptr->ai_addr);
                if (::connect(m_SocketFD, reinterpret_cast<::sockaddr *>(sockaddr), sizeof(sockaddr_in)) < 0)
                    continue;
                m_RemoteAddr = *sockaddr;
                break;
            }

            freeaddrinfo(result);

            if (!ptr)
                return false;

            m_Connected = true;
            m_RemoteIP = address;
            m_Port = port;
            return true;
        }

        size_t TCPSocket::send(const unsigned char *data, size_t size)
        {
            if (!m_Connected)
                return 0;

            size_t sent = 0;

            while (sent < size)
            {
                int cur = ::send(m_SocketFD, reinterpret_cast<const char *>(data + sent), size - sent, 0);
                if (cur <= 0)
                {
                    disconnect();
                    return 0;
                }
                sent += cur;
            }

            return sent;
        }

        std::size_t TCPSocket::receive(Buffer &buffer, std::size_t amount)
        {
            buffer.resizeBuffer(amount);
            buffer.setReadOffset(0);

            int recvAmount = ::recv(m_SocketFD, reinterpret_cast<char *>(buffer.getBufferData()), amount, MSG_DONTWAIT);
            if (recvAmount <= 0)
            {
#ifdef _WIN32
                int err = WSAGetLastError();
#else
                int err = errno;
#endif
                if (err == WOULDBLOCK)
                {
                    buffer.clearBuffer();
                    return 0;
                }

                disconnect();
                buffer.clearBuffer();
                return 0;
            }
            buffer.resizeBuffer(recvAmount);
            return recvAmount;
        }

        void TCPSocket::disconnect()
        {
            if (m_SocketFD != -1)
                closesocket(m_SocketFD);
            m_Connected = false;
        }

        bool TCPSocket::setBlocking(bool blocking)
        {
            return Internal::SetBlocking(m_SocketFD, blocking);
        }

        void InitialiseNetwork()
        {
#ifdef _WIN32
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
        }

        void ReleaseNetwork()
        {
#ifdef _WIN32
            WSACleanup();
#endif
        }

        namespace Internal
        {
            bool SetBlocking(int socket, bool blocking)
            {
#ifdef _WIN32
                unsigned long io_mode = !blocking;
                if (::ioctlsocket(socket, FIONBIO, &io_mode) != NO_ERROR)
                    return false;
#else
                const int flags = fcntl(socket, F_GETFL, 0);
                // checks if the socket is already in the right mode
                if ((flags & O_NONBLOCK) && !blocking)
                    return true;
                if (!(flags & O_NONBLOCK) && blocking)
                    return true;
                // and apply modifications if it's not the case
                if (::fcntl(socket, F_SETFL, blocking ? (flags ^ O_NONBLOCK) : (flags | O_NONBLOCK)) < 0)
                    return false;
#endif
                return true;
            }
        } // namespace Internal
    }     // namespace Network
} // namespace Soundux
