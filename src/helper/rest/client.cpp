#include "client.hpp"

#include <core/global/globals.hpp>
#include <httplib.h>
#include <sstream>

namespace Soundux
{
    namespace Objects
    {

        using namespace httplib;

        void SounduxClient::playSound(std::uint32_t soundID)
        {
            Client cl("localhost", Globals::gConfig.settings.serverPort);
            std::ostringstream stringStream;
            stringStream << "/playsound/" << soundID;
            cl.Get(stringStream.str().c_str());
        }
        void SounduxClient::stopSounds()
        {
            Client cl("localhost", Globals::gConfig.settings.serverPort);
            cl.Get("/stopsounds");
        }

    } // namespace Objects
} // namespace Soundux