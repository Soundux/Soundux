#pragma once
#include <string>

class SystemInfo
{
  private:
    static std::string getSystemInfo();
    static std::string getSettingsInfo();

  public:
    static std::string getSummary();
};