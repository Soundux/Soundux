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

            inline void onKeyEvent(int key)
            {
                // TODO: Find sound that uses that hotkey
            }
        } // namespace internal
    }     // namespace Hooks
} // namespace Soundux