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
        void Hotkeys::shouldNotify(bool status)
        {
            notify = status;
        }
        void Hotkeys::onKeyUp(int key)
        {
            if (notify && !pressedKeys.empty())
            {
                Globals::gGui->onHotKeyReceived(pressedKeys);
                pressedKeys.clear();
            }
            else
            {
                pressedKeys.erase(std::remove_if(pressedKeys.begin(), pressedKeys.end(),
                                                 [key](const auto &item) { return key == item; }),
                                  pressedKeys.end());
            }
        }
        void Hotkeys::onKeyDown(int key)
        {
            pressedKeys.emplace_back(key);
            if (pressedKeys == Globals::gSettings.stopHotkey)
            {
                Globals::gGui->stopSounds();
                return;
            }
            if (Globals::gSettings.tabHotkeysOnly)
            {
                auto tab = Globals::gData.getTab(Globals::gSettings.selectedTab);
                if (tab)
                {
                    auto sound = std::find_if(tab->sounds.begin(), tab->sounds.end(),
                                              [&](auto &item) { return item.hotkeys == pressedKeys; });
                    if (sound != tab->sounds.end())
                    {
                        auto pSound = Globals::gGui->playSound(sound->id);
                        if (pSound)
                        {
                            Globals::gGui->onSoundPlayed(*pSound);
                        }
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
                    auto pSound = Globals::gGui->playSound(sound->first);
                    if (pSound)
                    {
                        Globals::gGui->onSoundPlayed(*pSound);
                    }
                }
            }
        }
        std::string Hotkeys::getKeySequence(const std::vector<int> &keys)
        {
            std::string rtn;
            for (const auto &key : keys)
            {
                rtn += getKeyName(key) + " + ";
            }
            if (!rtn.empty())
            {
                return rtn.substr(0, rtn.length() - 3);
            }
            return "";
        }
    } // namespace Objects
} // namespace Soundux