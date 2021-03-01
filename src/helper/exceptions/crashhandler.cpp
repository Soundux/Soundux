#include "crashhandler.hpp"
#include <exception>
#include <fancy.hpp>

#if defined(__linux__)
#include <csignal>
#endif

void CrashHandler::init()
{
    std::set_terminate(terminateCallback);
#if defined(__linux__)
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGFPE, signalHandler);
#endif
}

void CrashHandler::terminateCallback()
{
    Fancy::fancy.logTime().failure() << "An exception crashed the program" << std::endl;

    try
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (const std::exception &exception)
    {
        Fancy::fancy.logTime().failure() << "Exception: " >> exception.what() << std::endl;
        Fancy::fancy.logTime().failure() << "Exception Type: " >> typeid(exception).name() << std::endl;
    }
    catch (...)
    {
        Fancy::fancy.logTime().failure() << "Exception: " >> "unknown" << std::endl;
    }

    backtrace();
}