#include "cli.hpp"

#include <core/global/globals.hpp>
#include <fancy.hpp>

#include <unordered_map>

namespace Soundux
{
    namespace Objects
    {

        typedef std::function<void(int, const char **)> CommandFunction;

        struct Command
        {
            std::string description, example;
            CommandFunction execFunction;
            Command(const std::string &cmdDescription, const std::string &cmdExample,
                    const CommandFunction &cmdFunction)
                : description(cmdDescription), example(cmdExample), execFunction(cmdFunction)
            {
            }
        };

        void playsoundCommand(int, const char **);
        void stopsoundsCommand(int, const char **);

        static const std::unordered_map<std::string, Command> commandMap{
            // command name      command description               command example
            {"playsound", {"play a sound specified by its id", "soundux playsound 40", playsoundCommand}},
            {"stopsounds", {"stop playing all of the current sounds", "soundux stopsounds", stopsoundsCommand}},
        };

        void displayHelp()
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
                execFunction(argc, args);
                return true;
            }
            return false;
        }

        void playsoundCommand(int argc, const char **args)
        {
            if (argc < 3)
            {
                Fancy::fancy.failure() << "Missing argument for command playsound" << std::endl;
                displayHelp();
                return;
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
                return;
            }
            Globals::gClient.playSound(soundID);
        }

        void stopsoundsCommand(int, const char **)
        {
            Globals::gClient.stopSounds();
        }

    } // namespace Objects
} // namespace Soundux
