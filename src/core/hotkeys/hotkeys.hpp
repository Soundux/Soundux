#pragma once
#include "keys.hpp"
#include <atomic>
#include <libremidi/libremidi.hpp>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        class Hotkeys
        {
            libremidi::midi_in midi;

          protected:
            Hotkeys() = default;
            virtual bool setup();

          protected:
            std::vector<Key> pressedKeys;
            std::atomic<bool> shouldNotify = false;

          public:
            static std::shared_ptr<Hotkeys> createInstance();

          public:
            virtual void notify(bool);

            virtual void onKeyUp(const Key &);
            virtual void onKeyDown(const Key &);

            virtual void pressKeys(const std::vector<Key> &) = 0;
            virtual void releaseKeys(const std::vector<Key> &) = 0;

            virtual std::string getKeyName(const Key &);
            virtual std::string getKeySequence(const std::vector<Key> &);
        };
    } // namespace Objects
} // namespace Soundux