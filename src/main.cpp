#include <backward.hpp>
#include <core/enums/enums.hpp>
#include <core/global/globals.hpp>
#include <fancy.hpp>
#include <guard.hpp>
#include <ui/impl/webview/webview.hpp>

#if defined(__linux__)
#include <helper/audio/linux/backend.hpp>
#endif

#if defined(_WIN32)
#include "../assets/icon.h"
#include <Windows.h>
#include <helper/misc/misc.hpp>
#include <shellapi.h>

int __stdcall WinMain([[maybe_unused]] HINSTANCE hInstrance, [[maybe_unused]] HINSTANCE prevInstance,
                      [[maybe_unused]] LPSTR winArgs, [[maybe_unused]] int argc)
#else
int main(int argc, char **arguments)
#endif
{
    using namespace Soundux::Globals; // NOLINT
    using namespace Soundux::Objects; // NOLINT
    using namespace Soundux::Enums;   // NOLINT

#if defined(_WIN32)
    auto **arguments = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::vector<std::string> args;
    for (int i = 0; argc > i; i++)
    {
        args.emplace_back(Soundux::Helpers::narrow(arguments[i]));
    }
#else
    std::vector<std::string> args(reinterpret_cast<char **>(arguments), reinterpret_cast<char **>(arguments) + argc);
#endif

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

    backward::SignalHandling crashHandler;
    Instance::Guard guard("soundux-guard");

    if (guard.isAnotherRunning())
    {
        Fancy::fancy.logTime().failure() << "Another Instance is already running!" << std::endl;
        return 1;
    }

    gConfig.load();
    gData.set(gConfig.data);
    gSettings = gConfig.settings;

#if defined(__linux__)
    gIcons = gIcons->createInstance();
    gAudioBackend = AudioBackend::createInstance(gSettings.audioBackend);
#endif

    gAudio.setup();
    gYtdl.setup();

#if defined(__linux__)
    if (gAudioBackend && gSettings.audioBackend == BackendType::PulseAudio && gConfig.settings.useAsDefaultDevice)
    {
        gAudioBackend->useAsDefault();
    }
#endif

    gGui = std::make_unique<Soundux::Objects::WebView>();
    gGui->setup();

    if (std::find(args.begin(), args.end(), "--hidden") == args.end())
    {
        gGui->show();
    }
    else
    {
        Fancy::fancy.logTime().message() << "Starting window hidden" << std::endl;
    }

#if defined(_WIN32)
    HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1));
    SendMessage(GetActiveWindow(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
    SendMessage(GetActiveWindow(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
#endif

    gGui->mainLoop();

    gAudio.destroy();
#if defined(__linux__)
    if (gAudioBackend)
    {
        gAudioBackend->destroy();
    }
#endif
    gConfig.data.set(gData);
    gConfig.settings = gSettings;
    gConfig.save();

    return 0;
}