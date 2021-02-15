#pragma once
#include "../core/global/objects.hpp"
#include "../helper/audio/audio.hpp"
#include <cstdint>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class Window
        {
          protected:
            virtual void addTab();
            virtual std::vector<Sound> getTabSounds(const Tab &) const;

          public:
            ~Window();
            virtual void setup();
            virtual void mainLoop() = 0;

            virtual void onHotKeyReceived(std::vector<std::string>) = 0;
        };
    } // namespace Objects
} // namespace Soundux