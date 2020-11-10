/*

    Will use SetWindowsHookEx with WH_KEYBOARD_LL
    To find the sound that uses this hotkey, we will have a map of all sounds that have hotkeys bound to them, the key
    of that map will be the last key that has to be pressed for the sound to play, then we check all the previous keys
    with GetAsyncKeyState ( if that works, if it doesn't well save the keystates from the KeyBoard hook in a map )
    The Hotkey-Map should be able to be used without a mutex - since the only time we access it outside of the keyboard
    thread is the time we load all the sounds and at that time the keyboard hook doesn't even run.

*/
#ifdef _WIN32
#pragma once
#include <Windows.h>
#include <iostream>
#include "global.h"
#include <thread>
#include <atomic>

namespace Soundux
{
    namespace Hooks
    {
        namespace internal
        {
            inline HHOOK oKeyBoardProc;
            inline std::thread keyListener;
            inline std::atomic<bool> killThread = false;

            inline LRESULT CALLBACK LLkeyBoardProc(int nCode, WPARAM wParam, LPARAM lParam)
            {
                if (nCode == HC_ACTION)
                {
                    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
                    {
                        PKBDLLHOOKSTRUCT info = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);
                        internal::onKeyEvent(info->vkCode, true);
                    }
                    else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
                    {
                        PKBDLLHOOKSTRUCT info = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);
                        internal::onKeyEvent(info->vkCode, false);
                    }
                }
                return CallNextHookEx(oKeyBoardProc, nCode, wParam, lParam);
            }
        } // namespace internal
        inline void setup()
        {
            internal::keyListener = std::thread([&] {
                internal::oKeyBoardProc = SetWindowsHookEx(WH_KEYBOARD_LL, internal::LLkeyBoardProc, NULL, NULL);

                MSG message;
                while (!internal::killThread.load())
                {
                    PeekMessage(&message, 0, 0, 0, PM_REMOVE);
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                UnhookWindowsHookEx(internal::oKeyBoardProc);
            });
        }
        inline void stop()
        {
            internal::killThread.store(true);
            internal::keyListener.join();
        }

        std::string getKeyName(const int key)
        {
            UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);

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

            return name;
        }
    } // namespace Hooks
} // namespace Soundux
#endif