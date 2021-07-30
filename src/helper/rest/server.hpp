#pragma once

#include <memory>
#include <thread>

// forward to avoid strange compilation errors
namespace httplib
{
    class Server;
}

namespace Soundux
{
    namespace Objects
    {
        class SounduxServer
        {
            std::shared_ptr<httplib::Server> m_Server;
            std::thread m_ListenThread;

          public:
            void start();
            void stop();

          private:
            void bindFunctions();
        };
    } // namespace Objects
} // namespace Soundux