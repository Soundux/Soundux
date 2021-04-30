#include "server.hpp"
#include <core/global/globals.hpp>
#include <fancy.hpp>

namespace Soundux
{
    namespace Objects
    {

        using namespace httplib;

        void SounduxServer::start()
        {
            if (!m_Server.bind_to_port("0.0.0.0", Globals::gConfig.settings.serverPort))
            {
                Fancy::fancy.logTime().failure()
                    << "Failed to bind port " << Globals::gConfig.settings.serverPort << std::endl;
                return;
            }
            bindFunctions();
            m_ListenThread = std::thread([this]() { m_Server.listen_after_bind(); });
            Fancy::fancy.logTime().success()
                << "Started server at port " << Globals::gConfig.settings.serverPort << std::endl;
        }

        void SounduxServer::stop()
        {
            m_Server.stop();
            m_ListenThread.join();
            Fancy::fancy.logTime().success() << "Stopped server" << std::endl;
        }

        void SounduxServer::bindFunctions()
        {
            m_Server.Get("/stopsounds", [](const Request &, Response &) {
                if (!Globals::gAudio.getPlayingSounds().empty())
                {
                    Globals::gGui->stopSounds();
                }
            });
            m_Server.Get(R"(/playsound/(\d+))", [](const Request &req, Response &) {
                std::string number = req.matches[1].str();
                uint32_t soundID;
                try
                {
                    soundID = std::stoi(number);
                }
                catch (std::exception &e)
                {
                    Fancy::fancy.logTime().message() << "Received invalid request" << req.path << std::endl;
                    return;
                }
                auto pSound = Globals::gGui->playSound(soundID);
                if (pSound)
                {
                    Globals::gGui->onSoundPlayed(*pSound);
                }
            });
        }
    } // namespace Objects
} // namespace Soundux