#pragma once

class CrashHandler
{
  private:
    static void terminateCallback();
    static void backtrace();

#if defined(__linux__)
    static void signalHandler(int);
#endif

  public:
    static void init();
};