#include "hotkeys.hpp"
#include "../global/globals.hpp"

namespace Soundux
{
    namespace Objects
    {
        void Hotkeys::init()
        {
            listener = std::thread([this] { listen(); });
        }
        void Hotkeys::stop()
        {
            kill = true;
            listener.join();
        }
        void Hotkeys::shouldNotify(bool status)
        {
            notify = status;
        }
        void Hotkeys::onKeyUp([[maybe_unused]] int key)
        {
            if (notify && !pressedKeys.empty())
            {
                Globals::gGui->onHotKeyReceived(pressedKeys);
            }
            pressedKeys.clear();
        }
        void Hotkeys::onKeyDown(int key)
        {
            pressedKeys.emplace_back(key);
            if (Globals::gSettings.tabHotkeysOnly)
            {
                auto tab = Globals::gData.getTab(Globals::gSettings.selectedTab);
                if (tab)
                {
                    auto sound = std::find_if(tab->sounds.begin(), tab->sounds.end(),
                                              [&](auto &item) { return item.hotkeys == pressedKeys; });
                    if (sound != tab->sounds.end())
                    {
                        Globals::gGui->playSound(sound->id);
                    }
                }
            }
            else
            {
                std::shared_lock lock(Globals::gSoundsMutex);
                if (auto sound =
                        std::find_if(Globals::gSounds.begin(), Globals::gSounds.end(),
                                     [&](const auto &item) { return item.second.get().hotkeys == pressedKeys; });
                    sound != Globals::gSounds.end())
                {
                    Globals::gGui->playSound(sound->first);
                }
            }
        }
    } // namespace Objects
} // namespace Soundux