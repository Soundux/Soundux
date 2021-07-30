#if defined(_WIN32)
#include "windows.hpp"
#include <core/global/globals.hpp>
#include <winuser.h>

namespace Soundux::Objects
{
    HHOOK WindowsHotkeys::oMouseProc;
    HHOOK WindowsHotkeys::oKeyboardProc;

    void WindowsHotkeys::setup()
    {
        Hotkeys::setup();

        oMouseProc = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, nullptr, NULL);
        oKeyboardProc = SetWindowsHookEx(WH_KEYBOARD_LL, keyBoardProc, nullptr, NULL);

        listener = std::thread([this] { listen(); });
        keyPresser = std::thread([this] { presser(); });
    }
    LRESULT CALLBACK WindowsHotkeys::keyBoardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION)
        {
            auto *info = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam); // NOLINT

            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            {
                Key key;
                key.type = Enums::KeyType::Keyboard;
                key.key = static_cast<int>(info->vkCode);

                Globals::gHotKeys->onKeyDown(key);
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                Key key;
                key.type = Enums::KeyType::Keyboard;
                key.key = static_cast<int>(info->vkCode);

                Globals::gHotKeys->onKeyUp(key);
            }
        }
        return CallNextHookEx(oKeyboardProc, nCode, wParam, lParam);
    }
    LRESULT CALLBACK WindowsHotkeys::mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION)
        {
            switch (wParam)
            {
            case WM_RBUTTONUP: {
                Key key;
                key.key = VK_RBUTTON;
                key.type = Enums::KeyType::Mouse;
                Soundux::Globals::gHotKeys->onKeyUp(key);
            }
            break;
            case WM_RBUTTONDOWN: {
                Key key;
                key.key = VK_RBUTTON;
                key.type = Enums::KeyType::Mouse;
                Soundux::Globals::gHotKeys->onKeyDown(key);
            }
            break;
            case WM_MBUTTONUP: {
                Key key;
                key.key = VK_MBUTTON;
                key.type = Enums::KeyType::Mouse;
                Soundux::Globals::gHotKeys->onKeyUp(key);
            }
            break;
            case WM_MBUTTONDOWN: {
                Key key;
                key.key = VK_RBUTTON;
                key.type = Enums::KeyType::Mouse;
                Soundux::Globals::gHotKeys->onKeyDown(key);
            }
            break;
            }
        }
        return CallNextHookEx(oMouseProc, nCode, wParam, lParam);
    }
    void WindowsHotkeys::listen()
    {
        MSG message;
        while (!GetMessage(&message, nullptr, 0, 0))
        {
            if (kill)
            {
                return;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
    void WindowsHotkeys::presser()
    {
        std::unique_lock lock(keysToPressMutex);
        while (!kill)
        {
            cv.wait(lock, [&]() { return !keysToPress.empty() || kill; });
            //* Yes, this is absolutely cursed. I tried to implement this by just sending the keydown event once but
            //* it does not work like that on windows, so I have to do this, thank you Microsoft, I hate you.
            for (const auto &key : keysToPress)
            {
                keybd_event(key.key, 0, 1, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    std::string WindowsHotkeys::getKeyName(const Key &key)
    {
        if (!Hotkeys::getKeyName(key).empty())
        {
            return Hotkeys::getKeyName(key);
        }

        if (key.type == Enums::KeyType::Keyboard)
        {
            char name[128];
            auto result = GetKeyNameTextA(MapVirtualKey(key.key, MAPVK_VK_TO_VSC) << 16, name, 128);

            if (result == 0)
            {
                return "KEY_" + std::to_string(key.key);
            }

            return name;
        }

        if (key.type == Enums::KeyType::Mouse)
        {
            return "MOUSE_" + std::to_string(key.key);
        }

        return "";
    }
    void WindowsHotkeys::pressKeys(const std::vector<Key> &keys)
    {
        std::unique_lock lock(keysToPressMutex);
        keysToPress = keys;
    }
    void WindowsHotkeys::releaseKeys(const std::vector<Key> &keys)
    {
        std::unique_lock lock(keysToPressMutex);
        for (const auto &key : keys)
        {
            for (auto it = keysToPress.begin(); it != keysToPress.end();)
            {
                if (*it == key)
                {
                    it = keysToPress.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }
    WindowsHotkeys::~WindowsHotkeys()
    {
        kill = true;
        PostThreadMessage(GetThreadId(listener.native_handle()), WM_QUIT, 0, 0);

        listener.join();
        cv.notify_all();
        keyPresser.join();

        UnhookWindowsHookEx(oMouseProc);
        UnhookWindowsHookEx(oKeyboardProc);
    }
} // namespace Soundux::Objects
#endif