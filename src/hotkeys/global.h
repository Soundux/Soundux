#pragma once
#include <map>
#include "../config/config.h"

namespace Soundux
{
    namespace Hooks
    {
        namespace internal
        {
            inline std::map<int, bool> pressedKeys;

            inline void onKeyEvent(int key, bool down)
            {
                pressedKeys[key] = down;

                if (down)
                {
                    if (Config::gConfig.tabHotkeysOnly)
                    {
                        for (auto &hk : Config::gConfig.tabs[Config::gConfig.currentTab].songs)
                        {
                            bool allPressed = true;
                            for (auto &key : hk.hotKeys)
                            {
                                if (!pressedKeys[key])
                                {
                                    allPressed = false;
                                    break;
                                }
                            }
                            if (allPressed)
                            {
                                // TODO: Play sound
                                break;
                            }
                        }
                    }
                    else
                    {
                        for (auto &tab : Config::gConfig.tabs)
                        {
                            for (auto &song : tab.songs)
                            {
                                bool allPressed = true;
                                for (auto &key : song.hotKeys)
                                {
                                    if (!pressedKeys[key])
                                    {
                                        allPressed = false;
                                        break;
                                    }
                                }
                                if (allPressed)
                                {
                                    // TODO: Play sound
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } // namespace internal
    }     // namespace Hooks
} // namespace Soundux