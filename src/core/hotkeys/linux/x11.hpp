#pragma once
#if defined(__linux__)
#include <core/hotkeys/hotkeys.hpp>
#include <thread>

struct _XDisplay; // NOLINT
using Display = _XDisplay;

namespace Soundux
{
    namespace Objects
    {
        class X11 : public Hotkeys
        {
            int major_op;
            Display *display;
            std::thread listener;
            std::atomic<bool> kill = false;

          private:
            void listen();
            void setup() override;

          public:
            ~X11();
            std::string getKeyName(const Key &key) override;
            void pressKeys(const std::vector<Key> &keys) override;
            void releaseKeys(const std::vector<Key> &keys) override;
        };
    } // namespace Objects
} // namespace Soundux

#endif