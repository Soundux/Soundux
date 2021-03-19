#pragma once
#include <httplib.h>

class VersionCheck
{
    static httplib::Client client;

  public:
    static bool isLatest();
};