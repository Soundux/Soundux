#pragma once
#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        class Hotkeys
        {
            std::thread listener;
            std::atomic<bool> kill = false;
            std::atomic<bool> notify = false;

            std::vector<int> pressedKeys;

          private:
            void listen();

          public:
            void init();
            void stop();
            void shouldNotify(bool);

            void onKeyUp(int);
            void onKeyDown(int);
            std::string getKeyName(const int &);
        };
    } // namespace Objects
} // namespace Soundux