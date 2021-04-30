#pragma once

#include <httplib.h>

namespace Soundux
{
    namespace Objects
    {
        class SounduxServer
        {
            httplib::Server m_Server;
            std::thread m_ListenThread;

          public:
            void start();
            void stop();

          private:
            void bindFunctions();
        };
    } // namespace Objects
} // namespace Soundux