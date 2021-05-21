#include "cli.hpp"

#include <core/global/globals.hpp>
#include <fancy.hpp>

namespace Soundux
{
    namespace Objects
    {

        CommandLineInterface::Command::Command(const std::string &cmdDescription, const std::string &cmdExample,
                                               const command_function_t &cmdFunction)
            : description(cmdDescription), example(cmdExample), execFunction(cmdFunction)
        {
        }

        void CommandLineInterface::displayHelp()
        {
            Fancy::fancy << "usage : soundux <command> [value]" << std::endl << std::endl;
            Fancy::fancy << "available commands :" << std::endl;
            for (auto pair : commandMap)
            {
                const std::string &cmdName = pair.first;
                const std::string &cmdDescription = pair.second.description;
                const std::string &cmdExample = pair.second.example;
                // using of std::cout to avoid fancy coloring
                std::cout << "\t" << cmdName.c_str() << "\t\t" << cmdDescription.c_str()
                          << "\t\tExample : " << cmdExample.c_str() << std::endl;
            }
            Fancy::fancy << std::endl
                         << "to launch Soundux normally just type: \"soundux\" without arguments" << std::endl;
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
                const std::string commandName = args[1];

                auto it = commandMap.find(commandName);
                if (it == commandMap.end())
                {
                    displayHelp();
                    return true;
                }
                auto execFunction = it->second.execFunction;
                return execFunction(argc, args);
            }
            return false;
        }

        bool CommandLineInterface::playSoundCommand(int argc, const char **args)
        {
            if (argc < 3)
            {
                Fancy::fancy.failure() << "Missing argument for command playsound" << std::endl;
                displayHelp();
                return true;
            }
            std::string commandValue = args[2];
            uint32_t soundID;
            try
            {
                soundID = std::stoi(commandValue);
            }
            catch (std::exception &e)
            {
                Fancy::fancy.failure() << commandValue << " is not a correct number" << std::endl;
                return true;
            }
            Globals::gClient.playSound(soundID);
            return true;
        }

        bool CommandLineInterface::stopSoundsCommand(int, const char **)
        {
            Globals::gClient.stopSounds();
            return true;
        }

        bool CommandLineInterface::hideCommand(int, const char **)
        {
            Globals::gClient.hideWindow();
            return true;
        }

        bool CommandLineInterface::showCommand(int, const char **)
        {
            Globals::gClient.showWindow();
            return true;
        }

    } // namespace Objects
} // namespace Soundux
