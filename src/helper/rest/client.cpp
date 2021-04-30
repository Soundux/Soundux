#include "client.hpp"

#include <core/global/globals.hpp>
#include <httplib.h>

namespace Soundux
{
    namespace Objects
    {

        using namespace httplib;

        void SounduxClient::playSound(std::uint32_t soundID)
        {
            Client cl("localhost", Globals::gConfig.settings.serverPort);
            cl.Get(std::string("/playsound/" + std::to_string(soundID)).c_str());
        }
        void SounduxClient::stopSounds()
        {
            Client cl("localhost", Globals::gConfig.settings.serverPort);
            cl.Get("/stopsounds");
        }

    } // namespace Objects
} // namespace Soundux