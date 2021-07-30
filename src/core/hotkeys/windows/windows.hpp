#pragma once
#if defined(_WIN32)
#include <core/hotkeys/hotkeys.hpp>
#include <mutex>
#include <thread>
#include <windows.h>

namespace Soundux
{
    namespace Objects
    {
        class WindowsHotkeys : public Hotkeys
        {
            std::thread listener;
            std::thread keyPresser;
            std::atomic<bool> kill = false;

            std::condition_variable cv;
            std::mutex keysToPressMutex;
            std::vector<Key> keysToPress;

          private:
            void listen();
            void presser();
            void setup() override;

          private:
            static HHOOK oMouseProc;
            static HHOOK oKeyboardProc;

            static LRESULT CALLBACK mouseProc(int, WPARAM, LPARAM);
            static LRESULT CALLBACK keyBoardProc(int, WPARAM, LPARAM);

          public:
            ~WindowsHotkeys();
            std::string getKeyName(const Key &key) override;
            void pressKeys(const std::vector<Key> &keys) override;
            void releaseKeys(const std::vector<Key> &keys) override;
        };
    } // namespace Objects
} // namespace Soundux

#endif