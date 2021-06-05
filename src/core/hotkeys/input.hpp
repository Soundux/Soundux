#pragma once
#include "hotkeys.hpp"
#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct InputDevice
        {
            std::string name;
            int id;       // file descriptor on Linux
            bool grabbed; // when grabbed other applications can't receive inputs
            void *data;
        };

        class CustomInput
        {
            std::thread listener;
            std::atomic<bool> kill = false;

            std::vector<InputDevice> availableDevices;
            std::vector<InputDevice> selectedDevices;

            std::vector<int> pressedKeys;
            std::vector<int> keysToPress;

            Hotkeys *hotkeys;

          private:
            void listen();

            bool pressKey(int, const InputDevice &);
            bool releaseKey(int, const InputDevice &);

          public:
            CustomInput(Hotkeys &hotkeysClass) : hotkeys(&hotkeysClass) {}

            bool init();
            void stop();
            void shouldNotify(bool);

            void pressKeys(const std::vector<int> &);
            void releaseKeys(const std::vector<int> &);

            void selectDevice(const InputDevice &device);
            void unselectDevice(const InputDevice &device);

            // blocking input from other applications
            bool grabDevice(InputDevice &device);
            // unblocking input from other applications
            bool ungrabDevice(InputDevice &device);

            const std::vector<InputDevice> getAvailableDevices();
            const std::vector<InputDevice> getSelectedDevices();
        };
    } // namespace Objects
} // namespace Soundux