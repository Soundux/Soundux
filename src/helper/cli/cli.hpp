#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace Soundux
{
    namespace Objects
    {
        constexpr auto _1 = std::placeholders::_1;
        constexpr auto _2 = std::placeholders::_2;

        class CommandLineInterface
        {

            struct Command
            {
                using command_function_t = std::function<bool(int, const char **)>;

                std::string description, example;
                command_function_t execFunction;

                Command(const std::string &cmdDescription, const std::string &cmdExample,
                        const command_function_t &cmdFunction);
            };

            std::unordered_map<std::string, Command> commandMap{
                // command name      command description               command example
                {"playsound",
                 {"play a sound specified by its id", "soundux playsound 40",
                  std::bind(&CommandLineInterface::playSoundCommand, this, _1, _2)}},
                {"stopsounds",
                 {"stop playing all of the current sounds", "soundux stopsounds",
                  std::bind(&CommandLineInterface::stopSoundsCommand, this, _1, _2)}},
                {"hide",
                 {"\thide Soundux window (if not hidden)", "soundux hide",
                  std::bind(&CommandLineInterface::hideCommand, this, _1, _2)}},
                {"show",
                 {"\tshow Soundux window (if not visible)", "soundux show",
                  std::bind(&CommandLineInterface::showCommand, this, _1, _2)}},
            };

            bool playSoundCommand(int, const char **);
            bool stopSoundsCommand(int, const char **);
            bool hideCommand(int, const char **);
            bool showCommand(int, const char **);
            void displayHelp();

          public:
            bool parseProgramArguments(int argc, const char **args);
        };
    } // namespace Objects
} // namespace Soundux
