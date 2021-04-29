#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Network
    {
        class Buffer
        {
            std::vector<unsigned char> m_Data;
            std::size_t m_ReadOffset;

          public:
            Buffer(Buffer &&other)
            {
                m_Data = std::move(other.m_Data);
                m_ReadOffset = other.m_ReadOffset;
            }
            Buffer &operator=(const Buffer &other) = default;
            Buffer() : m_ReadOffset(0) {}
            Buffer(const Buffer &other) = default;
            Buffer(const std::string &data) : m_Data(data.begin(), data.end()), m_ReadOffset(0) {}

            void setReadOffset(std::size_t offset)
            {
                m_ReadOffset = offset;
            }
            std::size_t getReadOffset() const
            {
                return m_ReadOffset;
            }
            size_t getBufferSize() const
            {
                return m_Data.size();
            }
            const unsigned char *getBufferData() const
            {
                return m_Data.data();
            }
            unsigned char *getBufferData()
            {
                return m_Data.data();
            }
            bool isEmpty() const
            {
                return m_Data.empty();
            }
            void clearBuffer()
            {
                m_Data.clear();
            }
            void resizeBuffer(std::size_t newSize)
            {
                m_Data.resize(newSize);
            }

            template <typename T> Buffer &operator<<(const T &data)
            {
                size_t dataSize = sizeof(data);
                size_t oldBufferSize = getBufferSize();
                resizeBuffer(oldBufferSize + dataSize);
                memcpy(m_Data.data() + oldBufferSize, &data, dataSize);
                return *this;
            }

            Buffer &operator<<(const std::string &data)
            {
                size_t dataSize = data.size() + 1;
                size_t oldBufferSize = getBufferSize();
                resizeBuffer(oldBufferSize + dataSize);
                strcpy((char *)m_Data.data() + oldBufferSize, data.c_str());
                return *this;
            }

            template <typename T> Buffer &operator>>(T &data)
            {
                size_t dataSize = sizeof(T);
                assert(m_ReadOffset + dataSize <= getBufferSize());
                data = *((T *)(m_Data.data() + m_ReadOffset));
                m_ReadOffset += dataSize;
                return *this;
            }

            Buffer &operator>>(std::string &data)
            {
                size_t strLength = strlen((const char *)m_Data.data() + m_ReadOffset) + 1;
                data.resize(strLength);
                std::copy(m_Data.begin() + m_ReadOffset, m_Data.end(), data.begin());
                m_ReadOffset += strLength;
                return *this;
            }
        };

        class TCPListener;

        class TCPSocket
        {
            std::string m_RemoteIP;
            uint16_t m_Port;
            sockaddr_in m_RemoteAddr{};
            int m_SocketFD;
            bool m_Connected;

          public:
            TCPSocket() : m_Port(0), m_SocketFD(-1), m_Connected(false) {}

            bool connect(const std::string &address, uint16_t port);
            std::size_t send(const unsigned char *data, std::size_t size);
            std::size_t send(const Buffer &buffer)
            {
                return send(buffer.getBufferData(), buffer.getBufferSize());
            }
            std::size_t receive(Buffer &buffer, std::size_t amount);
            void disconnect();
            bool setBlocking(bool blocking);

            const std::string &getRemoteIP() const
            {
                return m_RemoteIP;
            }
            uint16_t getRemotePort() const
            {
                return m_Port;
            }
            bool isConnected() const
            {
                return m_Connected;
            }

            friend class TCPListener;
        };

        void InitialiseNetwork();
        void ReleaseNetwork();

        namespace Internal
        {
            bool SetBlocking(int socket, bool blocking);
        } // namespace Internal

    } // namespace Network
} // namespace Soundux
