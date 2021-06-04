#pragma once
#include <cstdint>

namespace Soundux
{
    namespace Enums
    {
        enum class ErrorCode : std::uint8_t
        {
            FailedToPlay,
            FailedToSeek,
            FailedToPause,
            FailedToRepeat,
            FailedToResume,
            FailedToMoveToSink,
            SoundNotFound,
            FolderDoesNotExist,
            TabDoesNotExist,
            FailedToSetHotkey,
            FailedToStartPassthrough,
            FailedToMoveBack,
            FailedToMoveBackPassthrough,
            FailedToRevertDefaultSource,
            FailedToSetDefaultSource,
            YtdlNotFound,
            YtdlInvalidUrl,
            YtdlInvalidJson,
            YtdlInformationUnknown,
            FailedToDelete,
            FailedToSetCustomVolume,
            FailedToMute,
        };

        enum class SortMode : std::uint8_t
        {
            ModifiedDate_Ascending,
            ModifiedDate_Descending,
            Alphabetical_Ascending,
            Alphabetical_Descending,
        };

        enum class Theme : std::uint8_t
        {
            System,
            Dark,
            Light
        };

        enum class ViewMode : std::uint8_t
        {
            List,
            Grid,
            EmulatedLaunchpad,
        };

        enum class BackendType : std::uint8_t
        {
            None,
            PipeWire,
            PulseAudio,
        };
    } // namespace Enums
} // namespace Soundux