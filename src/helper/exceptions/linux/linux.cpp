#if defined(__linux__)
#include "../crashhandler.hpp"
#include <csignal>
#include <execinfo.h>
#include <fancy.hpp>

void CrashHandler::backtrace()
{
    Fancy::fancy.logTime().success() << "Backtrace available!" << std::endl;

    void *elements[20];
    auto size = ::backtrace(elements, 20);
    auto *stack = backtrace_symbols(elements, size);

    for (int i = 0; size > i; i++)
    {
        Fancy::fancy.logTime().message() << stack[i] << std::endl;
    }

    free(stack);
}

void CrashHandler::signalHandler(int signal)
{
    if (signal == SIGFPE)
    {
        Fancy::fancy.logTime().warning() << "This crash is probably related to a bad pulseaudio config!" << std::endl;
    }

    Fancy::fancy.logTime().failure() << "Received Signal: " << signal << std::endl;

    backtrace();
    exit(1); // NOLINT
}
#endif