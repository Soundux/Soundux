#include "cli.hpp"

#include <fancy.hpp>
#include <helper/protocol/packets.hpp>
#include <core/global/globals.hpp>

#include <unordered_map>

namespace Soundux
{
    namespace Objects
    {
        using namespace Network::Protocol;

        struct Command
        {
            PacketType type;
            std::string description, example;
            Command(PacketType cmdType, const std::string &cmdDescription, const std::string &cmdExample)
                : type(cmdType), description(cmdDescription), example(cmdExample)
            {
            }
        };

        static const std::unordered_map<std::string, Command> commandMap{
            // command name        command id             command description               command example
            {"playsoundid", {PacketType::PlaySoundID, "play a sound specified by its id", "soundux playsoundid 40"}},
            {"stopsounds", {PacketType::StopSounds, "stop playing all of the current sounds", "soundux stopsounds"}},
        };

        void displayHelp()
        {
            std::cout << "usage : soundux <command> [value]\n" << std::endl;
            std::cout << "available commands :" << std::endl;
            for (auto pair : commandMap)
            {
                const std::string &cmdName = pair.first;
                const std::string &cmdDescription = pair.second.description;
                const std::string &cmdExample = pair.second.example;
                std::cout << "\t" << cmdName << "\t\t" << cmdDescription << "\t\tExample : " << cmdExample << std::endl;
            }
            std::cout << "\nto launch Soundux normally just type: \"soundux\" without arguments" << std::endl;
        }

        bool CommandLineInterface::parseProgramArguments(int argc, const char **args)
        {
            if (argc >= 2)
            {
                const std::string_view firstArg = args[1];
                if (firstArg == "--help" || firstArg == "-help" || firstArg == "help")
                {
                    displayHelp();
                    return true;
                }
                const std::string_view commandName = args[1];
                std::string_view commandValue;
                if (argc >= 3)
                {
                    commandValue = args[2];
                }
                // checking if the argument is a command
                auto it = commandMap.find(commandName.begin());
                if (it == commandMap.end())
                {
                    Fancy::fancy.logTime().failure() << "Unknown command " << commandName << std::endl;
                    displayHelp();
                    return true;
                }
                PacketType cmdType = it->second.type;

                Network::Buffer data;

                switch (cmdType)
                {
                case PacketType::PlaySoundID: {
                    uint32_t soundID;
                    try
                    {
                        soundID = std::stoi(commandValue.begin());
                    }
                    catch (std::exception &e)
                    {
                        Fancy::fancy.logTime().failure() << commandValue << " is not a valid number" << std::endl;
                        displayHelp();
                        return true;
                    }
                    PlaySoundPacket packet(soundID);
                    data << packet;
                    break;
                }

                case PacketType::StopSounds: {
                    StopSoundsPacket packet;
                    data << packet;
                }

                default:
                    break;
                }

                if (!data.isEmpty())
                {
                    Network::InitialiseNetwork();
                    Network::TCPSocket socket;

                    if (!socket.connect("127.0.0.1", Globals::gConfig.settings.serverPort))
                    {
                        Fancy::fancy.logTime().failure()
                            << "Can't connect to local socket to send command" << std::endl;
                        return false;
                    }

                    socket.send(data);
                    socket.disconnect();

                    Fancy::fancy.logTime().success() << "Command sent" << std::endl;
                    Network::ReleaseNetwork();
                }
                return true;
            }
            return false;
        }
    } // namespace Objects
} // namespace Soundux
