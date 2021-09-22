#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace Soundux
{
    namespace Objects
    {
        constexpr auto _1 = std::placeholders::_1;

        class CommandLineInterface
        {

            struct Command
            {
                using command_function_t = std::function<bool(const std::vector<std::string> &)>;

                std::string description, example;
                command_function_t execFunction;

                Command(const std::string &cmdDescription, const std::string &cmdExample,
                        const command_function_t &cmdFunction);
            };

            std::unordered_map<std::string, Command> commandMap{
                // command name      command description               command example
                {"playsound",
                 {"play a sound specified by its id", "soundux playsound 40",
                  std::bind(&CommandLineInterface::playSoundCommand, this, _1)}},
                {"stopsounds",
                 {"stop playing all of the current sounds", "soundux stopsounds",
                  std::bind(&CommandLineInterface::stopSoundsCommand, this, _1)}},
                {"hide",
                 {"\thide Soundux window (if not hidden)", "soundux hide",
                  std::bind(&CommandLineInterface::hideCommand, this, _1)}},
                {"show", // BUG : if Soundux is launched hidden, sending this command crashes the window
                 {"\tshow Soundux window (if not visible)", "soundux show",
                  std::bind(&CommandLineInterface::showCommand, this, _1)}},
            };

            bool playSoundCommand(const std::vector<std::string> &args);
            bool stopSoundsCommand(const std::vector<std::string> &args);
            bool hideCommand(const std::vector<std::string> &args);
            bool showCommand(const std::vector<std::string> &args);
            void displayHelp();

          public:
            bool parseProgramArguments(const std::vector<std::string> &args);
        };
    } // namespace Objects
} // namespace Soundux
