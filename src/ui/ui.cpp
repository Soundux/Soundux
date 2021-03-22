#include "ui.hpp"
#include "../core/global/globals.hpp"
#include <cstdint>
#include <fancy.hpp>
#include <filesystem>
#include <nfd.hpp>
#include <optional>

#if defined(_WIN32)
#include <codecvt>
#include <locale>
#endif

namespace Soundux::Objects
{
    void Window::setup()
    {
        NFD::Init();
        Globals::gHotKeys.init();
        for (auto &tab : Globals::gData.getTabs())
        {
            tab.sounds = refreshTabSounds(tab);
            Globals::gData.setTab(tab.id, tab);
        }
    }
    Window::~Window()
    {
        NFD::Quit();
        Globals::gHotKeys.stop();
    }
    std::vector<Sound> Window::refreshTabSounds(const Tab &tab) const
    {
        if (std::filesystem::exists(tab.path))
        {
            std::vector<Sound> rtn;
            for (const auto &entry : std::filesystem::directory_iterator(tab.path))
            {
                std::filesystem::path file = entry;
                if (entry.is_symlink())
                {
                    file = std::filesystem::read_symlink(entry);
                    if (file.has_relative_path())
                    {
                        file = std::filesystem::canonical(tab.path / file);
                    }
                }

                auto extension = file.extension().u8string();
                std::transform(extension.begin(), extension.end(), extension.begin(),
                               [](char c) { return std::tolower(c); });
                if (extension != ".mp3" && extension != ".wav" && extension != ".flac")
                {
                    continue;
                }

                Sound sound;

                std::error_code ec;
                auto writeTime = std::filesystem::last_write_time(file, ec);
                if (!ec)
                {
                    sound.modifiedDate = writeTime.time_since_epoch().count();
                }
                else
                {
                    Fancy::fancy.logTime().warning() << "Failed to read lastWriteTime of " << file << std::endl;
                }

                sound.path = file.u8string();
#if defined(_WIN32)
                std::transform(sound.path.begin(), sound.path.end(), sound.path.begin(),
                               [](char c) { return c == '\\' ? '/' : c; });
#endif
                sound.name = file.stem().u8string();

                auto oldSound = std::find_if(tab.sounds.begin(), tab.sounds.end(),
                                             [&sound](const auto &item) { return item.path == sound.path; });

                if (oldSound != tab.sounds.end())
                {
                    sound.id = oldSound->id;
                    sound.hotkeys = oldSound->hotkeys;
                    sound.isFavorite = oldSound->isFavorite;
                }
                else
                {
                    sound.id = ++Globals::gData.soundIdCounter;
                }

                rtn.emplace_back(sound);
            }

            return rtn;
        }

        Fancy::fancy.logTime().warning() << "Path " >> tab.path << " does not exist" << std::endl;
        return {};
    }
    std::optional<Tab> Window::addTab() // NOLINT
    {
        nfdnchar_t *outpath = {};
        auto result = NFD::PickFolder(outpath, nullptr);
        if (result == NFD_OKAY)
        {
#if defined(_WIN32)
            std::wstring wpath(outpath);
            std::string path = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wpath.c_str()); // NOLINT
            std::transform(path.begin(), path.end(), path.begin(), [](char c) { return c == '\\' ? '/' : c; });
#else
            std::string path(outpath);
#endif
            NFD_FreePathN(outpath);

            if (std::filesystem::exists(path))
            {
                Tab tab;
                tab.path = path;
                tab.sounds = refreshTabSounds(tab);
                tab.name = std::filesystem::path(path).filename().u8string();

                tab = Globals::gData.addTab(std::move(tab));

                return tab;
            }
            Fancy::fancy.logTime().warning() << "Selected Folder does not exist!" << std::endl;
            onError(ErrorCode::FolderDoesNotExist);
        }
        return std::nullopt;
    }
#if defined(__linux__)
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            if (!Globals::gSettings.allowOverlapping)
            {
                Globals::gAudio.stopAll();
            }
            if (Globals::gSettings.muteDuringPlayback)
            {
                Globals::gPulse.muteLoopback(true);
            }

            if (Globals::gSettings.output.empty() && !Globals::gSettings.useAsDefaultDevice)
            {
                return Globals::gAudio.play(*sound);
            }
            if (Globals::gSettings.useAsDefaultDevice ||
                Globals::gPulse.moveApplicationsToSinkMonitor(Globals::gSettings.output))
            {
                auto playingSound = Globals::gAudio.play(*sound);
                auto remotePlayingSound = Globals::gAudio.play(*sound, Globals::gAudio.sinkAudioDevice, true);

                if (playingSound && remotePlayingSound)
                {
                    std::unique_lock lock(groupedSoundsMutex);
                    groupedSounds.insert({playingSound->id, remotePlayingSound->id});

                    return *playingSound;
                }

                if (playingSound)
                    stopSound(playingSound->id);
                if (remotePlayingSound)
                    stopSound(remotePlayingSound->id);
            }
            else
            {
                Fancy::fancy.logTime().failure() << "Failed to move Application '" << Globals::gSettings.output
                                                 << "' to soundux sink for sound " << id << std::endl;
                onError(ErrorCode::FailedToMoveToSink);
                return std::nullopt;
            }
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Sound " << id << " not found" << std::endl;
            onError(ErrorCode::SoundNotFound);
            return std::nullopt;
        }

        Fancy::fancy.logTime().failure() << "Failed to play sound " << id << std::endl;
        onError(ErrorCode::FailedToPlay);
        return std::nullopt;
    }
#else
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        auto device = Globals::gAudio.getAudioDevice(Globals::gSettings.output);

        if (sound && (Globals::gSettings.output.empty() || (device && device->get().isDefault)))
        {
            if (!Globals::gSettings.allowOverlapping)
            {
                Globals::gAudio.stopAll();
            }
            return Globals::gAudio.play(*sound);
        }
        if (sound && device)
        {
            if (!Globals::gSettings.allowOverlapping)
            {
                Globals::gAudio.stopAll();
            }
            auto playingSound = Globals::gAudio.play(*sound);
            auto remotePlayingSound = Globals::gAudio.play(*sound, *device, true);

            if (playingSound && remotePlayingSound)
            {
                std::unique_lock lock(groupedSoundsMutex);
                groupedSounds.insert({playingSound->id, remotePlayingSound->id});
                return *playingSound;
            }

            if (playingSound)
                stopSound(playingSound->id);
            if (remotePlayingSound)
                stopSound(remotePlayingSound->id);
        }
        else if (!sound)
        {
            Fancy::fancy.logTime().failure() << "Sound " << id << " not found" << std::endl;
            onError(ErrorCode::SoundNotFound);
            return std::nullopt;
        }
        Fancy::fancy.logTime().failure() << "Failed to play sound " << id << std::endl;
        onError(ErrorCode::FailedToPlay);
        return std::nullopt;
    }
#endif
    std::optional<PlayingSound> Window::pauseSound(const std::uint32_t &id)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.output.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            std::shared_lock lock(groupedSoundsMutex);
            if (groupedSounds.find(id) == groupedSounds.end())
            {
                if (!Globals::gSettings.output.empty() || !Globals::gSettings.useAsDefaultDevice)
                {
                    Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                }
            }
            else
            {
                remoteSoundId = groupedSounds.at(id);
            }
        }
        auto playingSound = Globals::gAudio.pause(id);
        if (remoteSoundId)
        {
            Globals::gAudio.pause(*remoteSoundId);
        }

        if (playingSound)
        {
            return *playingSound;
        }

        Fancy::fancy.logTime().warning() << "Failed to pause sound " << id << std::endl;
        onError(ErrorCode::FailedToPause);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::resumeSound(const std::uint32_t &id)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.output.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            std::shared_lock lock(groupedSoundsMutex);
            if (groupedSounds.find(id) == groupedSounds.end())
            {
                if (!Globals::gSettings.output.empty() || !Globals::gSettings.useAsDefaultDevice)
                {
                    Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                }
            }
            else
            {
                remoteSoundId = groupedSounds.at(id);
            }
        }
        auto playingSound = Globals::gAudio.resume(id);
        if (remoteSoundId)
        {
            Globals::gAudio.resume(*remoteSoundId);
        }

        if (playingSound)
        {
            return *playingSound;
        }

        Fancy::fancy.logTime().warning() << "Failed to resume sound " << id << std::endl;
        onError(ErrorCode::FailedToResume);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::seekSound(const std::uint32_t &id, std::uint64_t seekTo)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.output.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            std::shared_lock lock(groupedSoundsMutex);
            if (groupedSounds.find(id) == groupedSounds.end())
            {
                if (!Globals::gSettings.output.empty() || !Globals::gSettings.useAsDefaultDevice)
                {
                    Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                }
            }
            else
            {
                remoteSoundId = groupedSounds.at(id);
            }
        }
        auto playingSound = Globals::gAudio.seek(id, seekTo);
        if (remoteSoundId)
        {
            Globals::gAudio.seek(*remoteSoundId, seekTo);
        }

        if (playingSound)
        {
            return *playingSound;
        }

        Fancy::fancy.logTime().warning() << "Failed to seek sound " << id << " to " << seekTo << std::endl;
        onError(ErrorCode::FailedToSeek);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::repeatSound(const std::uint32_t &id, bool shouldRepeat)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.output.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            std::shared_lock lock(groupedSoundsMutex);
            if (groupedSounds.find(id) == groupedSounds.end())
            {
                if (!Globals::gSettings.output.empty() || !Globals::gSettings.useAsDefaultDevice)
                {
                    Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                }
            }
            else
            {
                remoteSoundId = groupedSounds.at(id);
            }
        }
        auto playingSound = Globals::gAudio.repeat(id, shouldRepeat);
        if (remoteSoundId)
        {
            Globals::gAudio.repeat(*remoteSoundId, shouldRepeat);
        }

        if (playingSound)
        {
            return *playingSound;
        }

        Fancy::fancy.logTime().failure() << "Failed to set repeat-state of sound " << id << " to " << shouldRepeat
                                         << std::endl;
        onError(ErrorCode::FailedToRepeat);
        return std::nullopt;
    }
    std::vector<Tab> Window::removeTab(const std::uint32_t &id)
    {
        Globals::gData.removeTabById(id);
        return Globals::gData.getTabs();
    }
    bool Window::stopSound(const std::uint32_t &id)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.output.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            std::shared_lock lock(groupedSoundsMutex);
            if (groupedSounds.find(id) == groupedSounds.end())
            {
                Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
            }
            else
            {
                remoteSoundId = groupedSounds.at(id);
            }
        }

        auto status = Globals::gAudio.stop(id);
        if (remoteSoundId)
        {
            Globals::gAudio.stop(*remoteSoundId);
        }

#if defined(__linux__)
        if (Globals::gAudio.getPlayingSounds().empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            if (!Globals::gPulse.moveBackCurrentApplications())
            {
                Fancy::fancy.logTime().failure()
                    << "Failed to move back current application, sound: " << id << std::endl;
                onError(ErrorCode::FailedToMoveBack);
            }
        }
#endif

        return status;
    }
    void Window::stopSounds()
    {
        Globals::gQueue.push_unique(0, []() { Globals::gAudio.stopAll(); });
#if defined(__linux__)
        if (!Globals::gPulse.moveBackCurrentApplications())
        {
            Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
            onError(ErrorCode::FailedToMoveBack);
        }
        if (!Globals::gPulse.moveBackApplicationsFromPassthrough())
        {
            Fancy::fancy.logTime().failure() << "Failed to move back current passthrough application" << std::endl;
            onError(ErrorCode::FailedToMoveBackPassthrough);
        }
#endif
    }
    void Window::changeSettings(const Settings &settings)
    {
#if defined(__linux__)
        if (!settings.useAsDefaultDevice && Globals::gSettings.useAsDefaultDevice)
        {
            if (!Globals::gPulse.revertDefaultSourceToOriginal())
            {
                Fancy::fancy.logTime().failure() << "Failed to move back default source" << std::endl;
                onError(ErrorCode::FailedToRevertDefaultSource);
            }
        }
        else if (settings.useAsDefaultDevice && !Globals::gSettings.useAsDefaultDevice)
        {
            Globals::gSettings.output = "";
            if (!Globals::gPulse.moveBackCurrentApplications())
            {
                Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                onError(ErrorCode::FailedToMoveBack);
            }
            if (!Globals::gPulse.setDefaultSourceToSoundboardSink())
            {
                onError(ErrorCode::FailedToSetDefaultSource);
            }
        }
        if (settings.output != Globals::gSettings.output)
        {
            if (!Globals::gPulse.moveBackCurrentApplications())
            {
                Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                onError(ErrorCode::FailedToMoveBack);
            }
            if (!settings.output.empty() && !Globals::gAudio.getPlayingSounds().empty())
            {
                Globals::gPulse.moveApplicationsToSinkMonitor(settings.output);
            }
        }
#endif
        Globals::gSettings = settings;
    }
    void Window::onHotKeyReceived([[maybe_unused]] const std::vector<int> &keys)
    {
        Globals::gHotKeys.shouldNotify(false);
    }
    std::optional<Tab> Window::refreshTab(const std::uint32_t &id)
    {
        auto tab = Globals::gData.getTab(id);
        if (tab)
        {
            tab->sounds = refreshTabSounds(*tab);
            auto newTab = Globals::gData.setTab(id, *tab);
            if (newTab)
            {
                return newTab;
            }
        }
        Fancy::fancy.logTime().failure() << "Failed to refresh tab " << id << " tab does not exist" << std::endl;
        onError(ErrorCode::TabDoesNotExist);
        return std::nullopt;
    }
    std::optional<Sound> Window::setHotkey(const std::uint32_t &id, const std::vector<int> &hotkeys)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            sound->get().hotkeys = hotkeys;
            return sound->get();
        }
        Fancy::fancy.logTime().failure() << "Failed to set hotkey for sound " << id << ", sound does not exist"
                                         << std::endl;
        onError(ErrorCode::FailedToSetHotkey);
        return std::nullopt;
    }
    std::string Window::getHotkeySequence(const std::vector<int> &hotkeys)
    {
        return Globals::gHotKeys.getKeySequence(hotkeys);
    }
    std::vector<Tab> Window::changeTabOrder(const std::vector<int> &newOrder)
    {
        std::vector<Tab> newTabs;
        newTabs.reserve(newOrder.size());

        for (auto tabId : newOrder)
        {
            newTabs.emplace_back(*Globals::gData.getTab(tabId));
        }
        Globals::gData.setTabs(newTabs);
        return Globals::gData.getTabs();
    }
#if defined(__linux__)
    std::vector<PulseRecordingStream> Window::getOutputs()
    {
        //* The frontend only uses the stream name and should only show multiple streams that belong to one application
        //* once. The backend (gPulse.getRecordingStreams()) will work with multiple instances, so we need to filter out
        //* duplicates here.
        auto streams = Globals::gPulse.getRecordingStreams();
        std::vector<PulseRecordingStream> uniqueStreams;
        for (auto &stream : streams)
        {
            auto item = std::find_if(std::begin(uniqueStreams), std::end(uniqueStreams),
                                     [&](const auto &_stream) { return stream.name == _stream.name; });
            if (item == std::end(uniqueStreams))
            {
#if defined(USE_WNCK)
                auto icon = Soundux::Globals::gIcons.getIcon(stream.pid);
                if (icon)
                {
                    stream.appIcon = *icon;
                }
#endif
                uniqueStreams.emplace_back(stream);
            }
        }

        return uniqueStreams;
    }
    std::vector<PulsePlaybackStream> Window::getPlayback()
    {
        auto streams = Globals::gPulse.getPlaybackStreams();
        std::vector<PulsePlaybackStream> uniqueStreams;
        for (auto &stream : streams)
        {
            auto item = std::find_if(std::begin(uniqueStreams), std::end(uniqueStreams),
                                     [&](const auto &_stream) { return stream.name == _stream.name; });
            if (item == std::end(uniqueStreams))
            {
#if defined(USE_WNCK)
                auto icon = Soundux::Globals::gIcons.getIcon(stream.pid);
                if (icon)
                {
                    stream.appIcon = *icon;
                }
#endif
                uniqueStreams.emplace_back(stream);
            }
        }

        return uniqueStreams;
    }
    bool Window::startPassthrough(const std::string &name)
    {
        if (Globals::gSettings.output.empty() ||
            Globals::gPulse.moveApplicationsToSinkMonitor(Globals::gSettings.output))
        {
            if (!Globals::gPulse.moveApplicationsToApplicationPassthrough(name))
            {
                Fancy::fancy.logTime().failure()
                    << "Failed to move application: " << name << " to passthrough" << std::endl;
                onError(ErrorCode::FailedToStartPassthrough);
                return false;
            }
            return true;
        }

        Fancy::fancy.logTime().failure() << "Failed to start passthrough for application: " << name << std::endl;
        onError(ErrorCode::FailedToStartPassthrough);
        return false;
    }
    void Window::stopPassthrough()
    {
        if (Globals::gAudio.getPlayingSounds().empty())
        {
            if (!Globals::gPulse.moveBackCurrentApplications())
            {
                Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                onError(ErrorCode::FailedToMoveBack);
            }
        }
        if (!Globals::gPulse.moveBackApplicationsFromPassthrough())
        {
            Fancy::fancy.logTime().failure() << "Failed to move back current passthrough application" << std::endl;
            onError(ErrorCode::FailedToMoveBackPassthrough);
        }
    }
#else
    std::vector<AudioDevice> Window::getOutputs()
    {
        Globals::gAudio.refreshAudioDevices();
        return Globals::gAudio.getAudioDevices();
    }
#endif
    void Window::onSoundFinished(const PlayingSound &sound)
    {
        std::unique_lock lock(groupedSoundsMutex);
        if (groupedSounds.find(sound.id) != groupedSounds.end())
        {
            groupedSounds.erase(sound.id);
        }
        lock.unlock();

#if defined(__linux__)
        if (Globals::gAudio.getPlayingSounds().size() == 1)
        {
            if (Globals::gSettings.muteDuringPlayback)
            {
                Globals::gPulse.muteLoopback(false);
            }
            if (!Globals::gPulse.currentlyPassingthrough())
            {
                if (!Globals::gPulse.moveBackCurrentApplications())
                {
                    Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                    onError(ErrorCode::FailedToMoveBack);
                }
            }
        }
#endif
    }
    std::vector<Sound> Window::getFavourites()
    {
        return Globals::gData.getFavorites();
    }
    std::optional<Sound> Window::markFavourite(const std::uint32_t &id, bool favourite)
    {
        return Globals::gData.markFavorite(id, favourite);
    }
} // namespace Soundux::Objects