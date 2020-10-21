/*

    On Windows Soundux will depend on VB-AudioCable

    To play sounds we will fetch the playback devices and allow the user to select *multiple* (to play on the speakers
    and vb-audio cable input) playback devices to play to.

*/
#pragma once
#include "global.h"

namespace Soundux
{
    namespace Playback
    {
        //! Windows doesn't require additional implementations.
    } // namespace Playback
} // namespace Soundux