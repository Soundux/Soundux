#if defined(__linux__) && __has_include(<libinput.h>)
#include "../input.hpp"
#include <chrono>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <libinput.h>
#include <libudev.h>
#include <linux/input.h>
#include <map>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;

using open_function_t = int (*)(const char *, int, void *);
using close_function_t = void (*)(int, void *);

namespace Soundux::Objects
{

    std::map<std::string, int> fdMap; // mapping events to fd

    int open_dev(const char *path, int flags, void *)
    {
        int fd = open(path, flags);
        fdMap.insert({path, fd});
        return fd < 0 ? -errno : fd;
    }

    void close_dev(int fd, void *)
    {
        close(fd);
    }

    // need it to find a device in a vector
    bool operator==(const InputDevice &device, const InputDevice &other)
    {
        return device.id == other.id;
    }

    void CustomInput::listen()
    {

        struct input_interface
        {
            open_function_t open_restricted = open_dev;
            close_function_t close_restricted = close_dev;
        } interface;

        struct libinput *li;
        struct libinput_event *event;

        udev *udevInstance = udev_new();

        li = libinput_udev_create_context(reinterpret_cast<libinput_interface *>(&interface), NULL, udevInstance);
        libinput_udev_assign_seat(li, "seat0");

        while (!kill)
        {

            libinput_dispatch(li);

            event = libinput_get_event(li);

            if (event == NULL)
            {
                std::this_thread::sleep_for(1ms); // waiting longer can cause input losses
                continue;
            }

            libinput_event_type eventType = libinput_event_get_type(event);

            if (eventType == LIBINPUT_EVENT_DEVICE_ADDED)
            {
                libinput_device *device = libinput_event_get_device(event);
                if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD) != 0)
                {
                    if (libinput_device_keyboard_has_key(device,
                                                         KEY_A)) // testing if the device has a key (A in this case)
                    {
                        const char *deviceName = libinput_device_get_name(device);
                        const char *deviceEventName = libinput_device_get_sysname(device);

                        InputDevice newDevice;
                        newDevice.name = deviceName;
                        newDevice.grabbed = false;
                        newDevice.data = device;
                        newDevice.id = fdMap[std::string("/dev/input/") + deviceEventName];

                        availableDevices.push_back(newDevice);
                    }
                    else
                    {
                        libinput_device_unref(device); // ignoring virtual keyboards
                    }
                }
                else
                {
                    libinput_device_unref(device); // ignoring devices that are not keyboards
                }
            }
            else if (eventType == LIBINPUT_EVENT_KEYBOARD_KEY)
            {
                libinput_event_keyboard *keyEvent = libinput_event_get_keyboard_event(event);
                uint32_t key = libinput_event_keyboard_get_key(keyEvent);
                libinput_key_state keyState = libinput_event_keyboard_get_key_state(keyEvent);

                libinput_device *device = libinput_event_get_device(event);

                // checking if the device that made the input is selected
                for (const auto &selectedDevice : selectedDevices)
                {
                    if (selectedDevice.data == device)
                    {
                        if (keyState == LIBINPUT_KEY_STATE_RELEASED)
                        {
                            hotkeys->onKeyUp(key + 8); // xserver add 8 to linux key codes
                        }
                        else if (keyState == LIBINPUT_KEY_STATE_PRESSED)
                        {
                            hotkeys->onKeyDown(key + 8); // xserver add 8 to linux key codes
                        }
                    }
                }
            }

            libinput_event_destroy(event);
        }

        libinput_unref(li);
    }

    bool CustomInput::init()
    {
        // checking permission to read kernel input devices
        std::fstream readTest("/dev/input/event0");
        if (!readTest) // to access this file the user can run the program as sudo or add himself in the input group by
                       // doing : "sudo usermod -a -G input $USER"
            return false;
        listener = std::thread([this] { listen(); });
        return true;
    }

    void CustomInput::stop()
    {
        kill = true;
        listener.join();
        fdMap.clear();
    }

    void CustomInput::pressKeys(const std::vector<int> &keys)
    {
        keysToPress = keys;
        for (const auto &key : keys)
        {
            for (const auto &device : selectedDevices)
            {
                if (!device.grabbed)
                {
                    pressKey(key, device);
                    break;
                }
            }
        }
    }

    void CustomInput::releaseKeys(const std::vector<int> &keys)
    {
        keysToPress.clear();
        for (const auto &key : keys)
        {
            for (const auto &device : selectedDevices)
            {
                if (!device.grabbed)
                {
                    releaseKey(key, device);
                    break;
                }
            }
        }
    }

    bool CustomInput::pressKey(int key, const InputDevice &device)
    {
        input_event fakeEvent;

        fakeEvent.code = key;
        fakeEvent.type = EV_KEY;
        fakeEvent.value = 1;

        int written = write(device.id, &fakeEvent, sizeof(fakeEvent));
        return !(written < 1);
    }

    bool CustomInput::releaseKey(int key, const InputDevice &device)
    {
        input_event fakeEvent;

        fakeEvent.code = key;
        fakeEvent.type = EV_KEY;
        fakeEvent.value = 0;

        int written = write(device.id, &fakeEvent, sizeof(fakeEvent));
        return !(written < 1);
    }

    bool CustomInput::grabDevice(InputDevice &device)
    {
        if (device.grabbed)
            return true;
        if (ioctl(device.id, EVIOCGRAB, (void *)1) < 0)
        {
            return false;
        }
        device.grabbed = true;
        return true;
    }

    bool CustomInput::ungrabDevice(InputDevice &device)
    {
        if (!device.grabbed)
            return true;
        if (ioctl(device.id, EVIOCGRAB, (void *)0) < 0)
        {
            return false;
        }
        device.grabbed = false;
        return true;
    }

    void CustomInput::selectDevice(const InputDevice &device)
    {
        if (std::find(selectedDevices.begin(), selectedDevices.end(), device) == selectedDevices.end())
        {
            selectedDevices.push_back(device);
        }
    }

    void CustomInput::unselectDevice(const InputDevice &device)
    {
        auto pos = std::find(selectedDevices.begin(), selectedDevices.end(), device);
        if (pos != selectedDevices.end())
        {
            selectedDevices.erase(pos);
        }
    }

} // namespace Soundux::Objects

#endif