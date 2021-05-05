#include "core/global/globals.hpp"
#include "helper/exceptions/crashhandler.hpp"
#include "ui/impl/webview/webview.hpp"
#include <InstanceGuard.hpp>
#include <fancy.hpp>

#if defined(__linux__)
#include <dlfcn.h>
#include <helper/audio/linux/pulse/pulse.hpp>
#endif

#if defined(_WIN32)
#include "../assets/icon.h"
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
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

    InstanceGuard::InstanceGuard guard("soundux-guard");
    if (guard.IsAnotherInstanceRunning())
    {
        Fancy::fancy.logTime().failure() << "Another Instance is already running!" << std::endl;
        return 1;
    }

#if defined(__linux__)
    Soundux::Globals::gIcons.setup();

    if (dlopen("libpulse.so", RTLD_LAZY))
    {
        Soundux::Globals::gAudioBackend = std::make_shared<Soundux::Objects::PulseAudio>();
        auto pulseBackend = std::dynamic_pointer_cast<Soundux::Objects::PulseAudio>(Soundux::Globals::gAudioBackend);
        pulseBackend->setup();

        if (!pulseBackend->switchOnConnectPresent())
        {
            pulseBackend->loadModules();
        }
    }

    Soundux::Globals::gAudio.setup();
#endif
    Soundux::Globals::gConfig.load();
    Soundux::Globals::gYtdl.setup();
#if defined(__linux__)
    if (Soundux::Globals::gConfig.settings.useAsDefaultDevice)
    {
        Soundux::Globals::gAudioBackend->useAsDefault();
    }
#endif
    Soundux::Globals::gData.set(Soundux::Globals::gConfig.data);
    Soundux::Globals::gSettings = Soundux::Globals::gConfig.settings;

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
    Soundux::Globals::gAudioBackend->destroy();
#endif
    Soundux::Globals::gConfig.data.set(Soundux::Globals::gData);
    Soundux::Globals::gConfig.settings = Soundux::Globals::gSettings;
    Soundux::Globals::gConfig.save();

    return 0;
}