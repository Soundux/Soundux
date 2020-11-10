#include "global.h"
#include "../core.h"
#include <chrono>

void Soundux::Hooks::internal::onKeyEvent(int key, bool down)
{
    // TODO: Check if window is focused.
    if (translateHotkeys)
    {
        if (down && !capturedKeyStates[key].first)
        {
            std::vector<std::tuple<int, bool, std::chrono::system_clock::time_point>> pressedStates;
            for (auto keyState : capturedKeyStates)
            {
                if (keyState.second.first)
                    pressedStates.push_back(
                        std::make_tuple(keyState.first, keyState.second.first, keyState.second.second));
            }

            pressedStates.push_back(std::make_tuple(key, down, std::chrono::system_clock::now()));

            std::sort(pressedStates.begin(), pressedStates.end(),
                      [](const auto &left, const auto &right) { return std::get<2>(left) < std::get<2>(right); });

            capturedKeyList.clear();
            for (auto &item : pressedStates)
            {
                capturedKeyList.push_back(std::get<0>(item));
            }

            QList<QString> stateStr;
            for (auto &item : pressedStates)
            {
                stateStr.push_back(QString::fromStdString(getKeyName(std::get<0>(item))));
            }
            emit gCore.keyPress(stateStr);
        }
        else if (!down)
        {
            emit gCore.keyCleared();
        }

        capturedKeyStates[key] = std::make_pair(down, std::chrono::system_clock::now());
        return;
    }

    pressedKeys[key] = down;

    if (down && !translateHotkeys)
    {
        // TODO: Detect Keyorder.
        if (Config::gConfig.tabHotkeysOnly)
        {
            for (auto &song : Config::gConfig.tabs[Config::gConfig.currentTab].songs)
            {
                bool allPressed = song.hotKeys.empty();
                for (auto &hotKey : song.hotKeys)
                {
                    if (!pressedKeys[hotKey])
                    {
                        allPressed = false;
                        break;
                    }
                }
                if (allPressed)
                {
                    gCore.playSong(song.path);
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
                    bool allPressed = song.hotKeys.empty();
                    for (auto &hotKey : song.hotKeys)
                    {
                        if (!pressedKeys[hotKey])
                        {
                            allPressed = false;
                            break;
                        }
                    }
                    if (allPressed)
                    {
                        gCore.playSong(song.path);
                        break;
                    }
                }
            }
        }
    }
}