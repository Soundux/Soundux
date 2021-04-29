#pragma once

namespace Soundux
{
    namespace Objects
    {
        class CommandLineInterface
        {
          public:
            bool parseProgramArguments(int argc, const char **args);
        };
    } // namespace Objects
} // namespace Soundux
