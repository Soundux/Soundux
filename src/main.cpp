#include "core/global/globals.hpp"
#include "helper/exceptions/crashhandler.hpp"
#include "ui/impl/webview/webview.hpp"
#include <InstanceGuard.hpp>
#include <fancy.hpp>

#if defined(_WIN32)
#include "../assets/icon.h"
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main(int argc, const char *args[])
#endif
{
#if defined(_WIN32)
    if (std::getenv("SOUNDUX_DEBUG"))
    {
        AllocConsole();
        freopen_s(reinterpret_cast<FILE **>(stdin), "CONIN$", "r", stdin);
        freopen_s(reinterpret_cast<FILE **>(stderr), "CONOUT$", "w", stderr);
        freopen_s(reinterpret_cast<FILE **>(stdout), "CONOUT$", "w", stdout);

        DWORD lMode = 0;
        HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleMode(hStdout, &lMode);
        SetConsoleMode(hStdout, lMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    }
#endif
    if (std::getenv("SOUNDUX_DEBUG") != nullptr) // NOLINT
    {
        Fancy::fancy.logTime().success() << "Enabling debug features" << std::endl;
    }

    CrashHandler::init();

    if (Soundux::Globals::gCli.parseProgramArguments(argc, args))
    {
        return 0;
    }

    InstanceGuard::InstanceGuard guard("soundux-guard");
    if (guard.IsAnotherInstanceRunning())
    {
        Fancy::fancy.logTime().failure() << "Another Instance is already running!" << std::endl;
        return 1;
    }

#if defined(__linux__)
    Soundux::Globals::gIcons.setup();

    if (!Soundux::Globals::gPulse.isSwitchOnConnectLoaded())
    {
        Soundux::Globals::gPulse.setup();
    }
    Soundux::Globals::gAudio.setup();
#endif
    Soundux::Globals::gConfig.load();
    Soundux::Globals::gYtdl.setup();
#if defined(__linux__)
    if (Soundux::Globals::gConfig.settings.useAsDefaultDevice)
    {
        Soundux::Globals::gPulse.setDefaultSourceToSoundboardSink();
    }
#endif
    Soundux::Globals::gData.set(Soundux::Globals::gConfig.data);
    Soundux::Globals::gSettings = Soundux::Globals::gConfig.settings;

    Soundux::Globals::gServer.init();

    Soundux::Globals::gGui = std::make_unique<Soundux::Objects::WebView>();
    Soundux::Globals::gGui->setup();
#if defined(_WIN32)
    HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1));
    SendMessage(GetActiveWindow(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
    SendMessage(GetActiveWindow(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
#endif
    Soundux::Globals::gGui->mainLoop();

    Soundux::Globals::gAudio.destroy();
#if defined(__linux__)
    Soundux::Globals::gPulse.destroy();
#endif
    Soundux::Globals::gConfig.data.set(Soundux::Globals::gData);
    Soundux::Globals::gConfig.settings = Soundux::Globals::gSettings;
    Soundux::Globals::gConfig.save();

    Soundux::Globals::gServer.destroy();

    return 0;
}