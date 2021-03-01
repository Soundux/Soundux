#if defined(_WIN32)
#include "../crashhandler.hpp"
#include <fancy.hpp>

void CrashHandler::backtrace()
{
    Fancy::fancy.logTime().failure() << "Backtrace is not available on Windows" << std::endl;
}
#endif