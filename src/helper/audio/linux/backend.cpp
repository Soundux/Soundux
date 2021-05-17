#if defined(__linux__)
#include "backend.hpp"
#include <fancy.hpp>

namespace Soundux::Objects
{
    void AudioBackend::setup()
    {
        Fancy::fancy.logTime().warning() << "setup(): not implemented (Possibly using null-audiobackend)" << std::endl;
    }
    void AudioBackend::destroy()
    {
        Fancy::fancy.logTime().warning() << "destroy(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
    }
    bool AudioBackend::useAsDefault()
    {
        Fancy::fancy.logTime().warning() << "useAsDefault(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return false;
    }
    bool AudioBackend::revertDefault()
    {
        Fancy::fancy.logTime().warning() << "revertDefault(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return false;
    }
    bool AudioBackend::muteInput([[maybe_unused]] bool state)
    {
        Fancy::fancy.logTime().warning() << "muteInput(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return false;
    }

    bool AudioBackend::stopPassthrough()
    {
        Fancy::fancy.logTime().warning() << "stopPassthrough(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return false;
    }
    bool AudioBackend::isCurrentlyPassingThrough()
    {
        Fancy::fancy.logTime().warning()
            << "isCurrentlyPassingThrough(): not implemented (Possibly using null-audiobackend)" << std::endl;
        return false;
    }
    bool AudioBackend::passthroughFrom([[maybe_unused]] std::shared_ptr<PlaybackApp> app) // NOLINT
    {
        Fancy::fancy.logTime().warning() << "passthroughFrom(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return false;
    }

    bool AudioBackend::stopSoundInput()
    {
        Fancy::fancy.logTime().warning() << "stopSoundInput(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return false;
    }

    bool AudioBackend::inputSoundTo([[maybe_unused]] std::shared_ptr<RecordingApp> app) // NOLINT
    {
        Fancy::fancy.logTime().warning() << "inputSoundTo(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return false;
    }

    std::shared_ptr<PlaybackApp> AudioBackend::getPlaybackApp([[maybe_unused]] const std::string &name)
    {
        Fancy::fancy.logTime().warning() << "getPlaybackApp(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return nullptr;
    }

    std::shared_ptr<RecordingApp> AudioBackend::getRecordingApp([[maybe_unused]] const std::string &name)
    {

        Fancy::fancy.logTime().warning() << "getRecordingApp(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return nullptr;
    }

    std::vector<std::shared_ptr<PlaybackApp>> AudioBackend::getPlaybackApps()
    {
        Fancy::fancy.logTime().warning() << "getPlaybackApps(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return {};
    }
    std::vector<std::shared_ptr<RecordingApp>> AudioBackend::getRecordingApps()
    {
        Fancy::fancy.logTime().warning() << "getRecordingApps(): not implemented (Possibly using null-audiobackend)"
                                         << std::endl;
        return {};
    }
} // namespace Soundux::Objects
#endif