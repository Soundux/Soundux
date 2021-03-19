#pragma once
#include <httplib.h>

class VersionCheck
{
    static httplib::Client client;

  public:
    static void setup();
    static bool isLatest();
};