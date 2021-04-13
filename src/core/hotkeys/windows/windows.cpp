#if defined(_WIN32)
#include "../hotkeys.hpp"
#include <Windows.h>
#include <chrono>
#include <core/global/globals.hpp>

using namespace std::chrono_literals;

namespace Soundux::Objects
{
    HHOOK oKeyBoardProc;
    HHOOK oMouseProc;

    LRESULT CALLBACK keyBoardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION)
        {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            {
                auto *info = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);
                Globals::gHotKeys.onKeyDown(info->vkCode);
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                auto *info = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);
                Globals::gHotKeys.onKeyUp(info->vkCode);
            }
        }
        return CallNextHookEx(oKeyBoardProc, nCode, wParam, lParam);
    }

    LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION)
        {
            // TODO(curve): How would I tell if XButton1 or XButton2 is pressed? Is there a nicer way to do this?

            switch (wParam)
            {
            case WM_RBUTTONUP:
                Globals::gHotKeys.onKeyUp(VK_RBUTTON);
                break;

            case WM_RBUTTONDOWN:
                Globals::gHotKeys.onKeyDown(VK_RBUTTON);
                break;

            case WM_MBUTTONDOWN:
                Globals::gHotKeys.onKeyDown(VK_MBUTTON);
                break;

            case WM_MBUTTONUP:
                Globals::gHotKeys.onKeyUp(VK_MBUTTON);
                break;
            }
        }
        return CallNextHookEx(oMouseProc, nCode, wParam, lParam);
    }

    void Hotkeys::listen()
    {
        oKeyBoardProc = SetWindowsHookEx(WH_KEYBOARD_LL, keyBoardProc, GetModuleHandle(nullptr), NULL);
        oMouseProc = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, GetModuleHandle(nullptr), NULL);
        keyPressThread = std::thread([this] {
            while (!kill)
            {
                //* Yes, this is absolutely cursed. I tried to implement this by just sending the keydown event once but
                //* it does not work like that on windows, so I have to do this, thank you Microsoft, I hate you.
                if (shouldPressKeys)
                {
                    for (const auto &key : keysToPress)
                    {
                        keybd_event(key, 0, 1, 0);
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        });

        MSG message;
        while (!GetMessage(&message, nullptr, NULL, NULL))
        {
            if (kill)
            {
                return;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    void Hotkeys::stop()
    {
        kill = true;
        UnhookWindowsHookEx(oMouseProc);
        UnhookWindowsHookEx(oKeyBoardProc);
        PostThreadMessage(GetThreadId(listener.native_handle()), WM_QUIT, 0, 0);
        listener.join();
        keyPressThread.join();
    }

    std::string Hotkeys::getKeyName(const int &key)
    {
        auto scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);

        CHAR name[128];
        int result = 0;
        switch (key)
        {
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_RCONTROL:
        case VK_RMENU:
        case VK_LWIN:
        case VK_RWIN:
        case VK_APPS:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_INSERT:
        case VK_DELETE:
        case VK_DIVIDE:
        case VK_NUMLOCK:
            scanCode |= KF_EXTENDED;
        default:
            result = GetKeyNameTextA(scanCode << 16, name, 128);
        }
        if (result == 0)
        {
            return "KEY_" + std::to_string(key);
        }

        return name;
    }

    void Hotkeys::pressKeys(const std::vector<int> &keys)
    {
        keysToPress = keys;
        shouldPressKeys = true;
    }

    void Hotkeys::releaseKeys([[maybe_unused]] const std::vector<int> &keys)
    {
        shouldPressKeys = false;
        keysToPress.clear();
        for (const auto &key : keys)
        {
            keybd_event(key, 0, 2, 0);
        }
    }
} // namespace Soundux::Objects
#endif