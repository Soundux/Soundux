#pragma once
#include "../core/global/objects.hpp"
#include <cstdint>
#include <string>

namespace Soundux
{
    namespace Objects
    {
        class Window
        {
            virtual void stopSounds();
            virtual void playSound(const std::uint32_t & /*soundId*/);
            virtual void stopSound(const std::uint32_t & /*playingSoundId*/);
            virtual void pauseSound(const std::uint32_t & /*playingSoundId*/);

            virtual void dataChanged(const Data &);
            virtual void changeSettings(const Settings &);
            virtual void changeLocalVolume(const std::uint8_t &);
            virtual void changeRemoteVolume(const std::uint8_t &);
            virtual void changeOutputDevice(const std::uint32_t &);

            virtual void addTab(); /* Should open a filedialog, will make use of tinyfiledialogs by default */
            virtual void removeTab(const std::uint32_t &);

            virtual void clearHotKey(const std::uint32_t & /* soundId */);
            virtual void requestHotKeyInput(const std::uint32_t & /* soundId */);

            /* Signals */
            virtual void onTabsChanged() = 0;
            virtual void onConfigLoaded() = 0;
            virtual void onFolderRefreshed() = 0;
            virtual void onError(const std::string &) = 0;
            virtual void onHotKeyChanged(const std::uint32_t & /* soundId */,
                                         const std::string &) = 0; //* Will trigger on KeyUp
            /*         */

            virtual std::string getHotKeyForSound(const std::uint32_t &) const;

            virtual void setup() = 0;
            virtual void mainLoop() = 0;
        };
    } // namespace Objects
} // namespace Soundux