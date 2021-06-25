#include "ui.hpp"
#include <core/global/globals.hpp>
#include <cstdint>
#include <fancy.hpp>
#include <filesystem>
#include <helper/audio/linux/backend.hpp>
#include <helper/audio/linux/pipewire/pipewire.hpp>
#include <helper/audio/linux/pulseaudio/pulseaudio.hpp>
#include <helper/misc/misc.hpp>
#include <nfd.hpp>
#include <optional>

namespace Soundux::Objects
{
    void Window::setup()
    {
        NFD::Init();
        Globals::gHotKeys = Hotkeys::createInstance();

        for (auto &tab : Globals::gData.getTabs())
        {
            tab.sounds = getTabContent(tab);
            Globals::gData.setTab(tab.id, tab);
        }

        setDevices();
    }
    void Window::setDevices()
    {
        for (const auto &device : AudioDevice::getDevices())
        {
            if (device.isDefault)
            {
                defaultPlayback = device;
            }
#if defined(__linux__)
            if (device.name == "soundux_sink")
            {
                remotePlayback = device;
            }
#endif
        }
    }
    Window::~Window()
    {
        NFD::Quit();
    }
    std::vector<Sound> Window::getTabContent(const Tab &tab) const
    {
#if defined(_WIN32)
        const auto path = Helpers::widen(tab.path);
#else
        const auto &path = tab.path;
#endif

        if (std::filesystem::exists(path))
        {
            std::vector<Sound> rtn;
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                std::filesystem::path file = entry;
                if (entry.is_symlink())
                {
                    file = std::filesystem::read_symlink(entry);
                    if (file.has_relative_path())
                    {
                        file = std::filesystem::canonical(path / file);
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
                    sound.localVolume = oldSound->localVolume;
                    sound.remoteVolume = oldSound->remoteVolume;
                }
                else
                {
                    sound.id = ++Globals::gData.soundIdCounter;
                }

                rtn.emplace_back(sound);
            }

            switch (tab.sortMode)
            {
            case Enums::SortMode::ModifiedDate_Descending:
                std::sort(rtn.begin(), rtn.end(), [](const auto &first, const auto &second) {
                    return first.modifiedDate > second.modifiedDate;
                });
                break;
            case Enums::SortMode::ModifiedDate_Ascending:
                std::sort(rtn.begin(), rtn.end(), [](const auto &first, const auto &second) {
                    return first.modifiedDate < second.modifiedDate;
                });
                break;
            case Enums::SortMode::Alphabetical_Descending:
                std::sort(rtn.begin(), rtn.end(),
                          [](const auto &first, const auto &second) { return first.name > second.name; });
                break;
            case Enums::SortMode::Alphabetical_Ascending:
                std::sort(rtn.begin(), rtn.end(),
                          [](const auto &first, const auto &second) { return first.name < second.name; });
                break;
            }

            return rtn;
        }

        Fancy::fancy.logTime().warning() << "Path " >> tab.path << " does not exist" << std::endl;
        return {};
    }
    std::vector<Tab> Window::addTab()
    {
#if defined(_WIN32)
        static std::wstring lastPath = Helpers::widen(std::getenv("USERPROFILE")); // NOLINT
#else
        static std::string lastPath = std::getenv("HOME"); // NOLINT
#endif

        nfdnchar_t *outpath = {};
        auto result = NFD::PickFolder(outpath, lastPath.empty() ? nullptr : lastPath.c_str());

        if (result == NFD_OKAY)
        {
#if defined(_WIN32)
            std::wstring path(outpath);
            std::transform(path.begin(), path.end(), path.begin(), [](wchar_t c) { return c == '\\' ? '/' : c; });
#else
            std::string path(outpath);
#endif
            NFD_FreePathN(outpath);

            if (std::filesystem::exists(path))
            {
#if defined(_WIN32)
                auto rootPath = Helpers::narrow(path);
#else
                const auto &rootPath = path;
#endif
                std::vector<Tab> tabs;
                lastPath = std::filesystem::path(path).parent_path();
#if defined(_WIN32)
                std::transform(lastPath.begin(), lastPath.end(), lastPath.begin(),
                               [](wchar_t c) { return c == '/' ? '\\' : c; });
#endif

                if (!Globals::gData.doesTabExist(rootPath))
                {
                    Tab rootTab;
                    rootTab.path = rootPath;
                    rootTab.sounds = getTabContent(rootTab);
                    rootTab.name = std::filesystem::path(rootPath).filename().u8string();

                    tabs.emplace_back(Globals::gData.addTab(std::move(rootTab)));
                }

                for (const auto &entry : std::filesystem::directory_iterator(path))
                {
                    if (entry.is_directory())
                    {
                        auto path = entry.path().u8string();
                        std::transform(path.begin(), path.end(), path.begin(),
                                       [](char c) { return c == '\\' ? '/' : c; });

                        const std::filesystem::path &subFolder(path);

                        if (!subFolder.empty() && !Globals::gData.doesTabExist(path))
                        {
                            Tab subFolderTab;
                            subFolderTab.path = path;
                            subFolderTab.sounds = getTabContent(subFolderTab);
                            subFolderTab.name = subFolder.filename().u8string();

                            if (!subFolderTab.sounds.empty())
                            {
                                tabs.emplace_back(Globals::gData.addTab(std::move(subFolderTab)));
                            }
                        }
                    }
                }

                return tabs;
            }
            Fancy::fancy.logTime().warning() << "Selected Folder does not exist!" << std::endl;
            onError(Enums::ErrorCode::FolderDoesNotExist);
        }

        return {};
    }
#if defined(__linux__)
    std::shared_ptr<PlayingSound> Window::playSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        auto groupedSounds = this->groupedSounds.write();

        if (sound)
        {
            if (!Globals::gSettings.allowOverlapping)
            {
                stopSounds();
            }
            if (Globals::gSettings.muteDuringPlayback)
            {
                if (Globals::gAudioBackend)
                {
                    if (!Globals::gAudioBackend->muteInput(true))
                    {
                        onError(Enums::ErrorCode::FailedToMute);
                    }
                }
            }
            if (!Globals::gSettings.pushToTalkKeys.empty())
            {
                Globals::gHotKeys->pressKeys(Globals::gSettings.pushToTalkKeys);
            }

            auto playingSound = PlayingSound::create(*sound, defaultPlayback);
            auto remotePlayingSound = PlayingSound::create(*sound, remotePlayback);

            if (playingSound && remotePlayingSound)
            {
                if (!Globals::gSettings.outputs.empty() && Globals::gAudioBackend)
                {
                    bool moveSuccess = false;
                    for (const auto &outputApp : Globals::gSettings.outputs)
                    {
                        if (Globals::gAudioBackend->inputSoundTo(Globals::gAudioBackend->getRecordingApp(outputApp)))
                        {
                            moveSuccess = true;
                        }
                    }

                    if (!moveSuccess)
                    {
                        if (playingSound)
                            playingSound->stop();
                        if (remotePlayingSound)
                            remotePlayingSound->stop();

                        onError(Enums::ErrorCode::FailedToMoveToSink);
                        return nullptr;
                    }
                }
            }
            else
            {
                Fancy::fancy.logTime().failure() << "Failed to play sound " << id << std::endl;
                onError(Enums::ErrorCode::FailedToPlay);
                return nullptr;
            }

            groupedSounds->insert({playingSound->getId(), {playingSound, remotePlayingSound}});
            return playingSound;
        }

        Fancy::fancy.logTime().failure() << "Sound " << id << " not found" << std::endl;
        onError(Enums::ErrorCode::SoundNotFound);
        return nullptr;
    }
#else
    // TODO
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            if (!Globals::gSettings.allowOverlapping)
            {
                stopSounds();
            }
            if (Globals::gSettings.muteDuringPlayback)
            {
                if (Globals::gWinSound && Globals::gWinSound->getMic())
                {
                    if (!Globals::gWinSound->getMic()->mute(true))
                    {
                        onError(Enums::ErrorCode::FailedToMute);
                    }
                }
            }
            if (!Globals::gSettings.pushToTalkKeys.empty())
            {
                Globals::gHotKeys->pressKeys(Globals::gSettings.pushToTalkKeys);
            }

            if (Globals::gSettings.outputs.empty() && !Globals::gSettings.useAsDefaultDevice)
            {
                return Globals::gAudio.play(*sound);
            }

            auto playingSound = Globals::gAudio.play(*sound);
            auto playbackDevice = Globals::gAudio.getAudioDevice(Globals::gSettings.outputs.front());

            if (playbackDevice && !playbackDevice->isDefault)
            {
                auto remotePlayingSound = Globals::gAudio.play(*sound, playbackDevice);
                if (playingSound && remotePlayingSound)
                {
                    groupedSounds->insert({playingSound->id, remotePlayingSound->id});
                    return *playingSound;
                }

                if (playingSound)
                    stopSound(playingSound->id);

                if (remotePlayingSound)
                    stopSound(remotePlayingSound->id);

                Fancy::fancy.logTime().failure() << "Failed to play sound " << id << std::endl;
                onError(Enums::ErrorCode::FailedToPlay);
                return std::nullopt;
            }

            return *playingSound;
        }

        Fancy::fancy.logTime().failure() << "Sound " << id << " not found" << std::endl;
        onError(Enums::ErrorCode::SoundNotFound);
        return std::nullopt;
    }
#endif
    std::shared_ptr<PlayingSound> Window::pauseSound(const std::uint32_t &id)
    {
        auto groupedSounds = this->groupedSounds.read();

        if (groupedSounds->count(id))
        {
            auto sounds = groupedSounds->at(id);

            if (sounds.first)
            {
                sounds.first->pause();
            }
            if (sounds.second)
            {
                sounds.second->pause();
            }

            return sounds.first;
        }

        Fancy::fancy.logTime().warning() << "Failed to pause sound " << id << std::endl;
        onError(Enums::ErrorCode::FailedToPause);
        return nullptr;
    }
    std::shared_ptr<PlayingSound> Window::resumeSound(const std::uint32_t &id)
    {
        auto groupedSounds = this->groupedSounds.read();

        if (groupedSounds->count(id))
        {
            auto sounds = groupedSounds->at(id);

            if (sounds.first)
            {
                sounds.first->resume();
            }
            if (sounds.second)
            {
                sounds.second->resume();
            }

            return sounds.first;
        }

        Fancy::fancy.logTime().warning() << "Failed to resume sound " << id << std::endl;
        onError(Enums::ErrorCode::FailedToResume);
        return nullptr;
    }
    std::shared_ptr<PlayingSound> Window::seekSound(const std::uint32_t &id, std::uint64_t seekTo)
    {
        auto groupedSounds = this->groupedSounds.read();

        if (groupedSounds->count(id))
        {
            auto sounds = groupedSounds->at(id);

            if (sounds.first)
            {
                sounds.first->seek(seekTo);
            }
            if (sounds.second)
            {
                sounds.second->seek(seekTo);
            }

            return sounds.first;
        }

        Fancy::fancy.logTime().warning() << "Failed to seek sound " << id << " to " << seekTo << std::endl;
        onError(Enums::ErrorCode::FailedToSeek);
        return nullptr;
    }
    std::shared_ptr<PlayingSound> Window::repeatSound(const std::uint32_t &id, bool shouldRepeat)
    {
        auto groupedSounds = this->groupedSounds.read();

        if (groupedSounds->count(id))
        {
            auto sounds = groupedSounds->at(id);

            if (sounds.first)
            {
                sounds.first->repeat(shouldRepeat);
            }
            if (sounds.second)
            {
                sounds.second->repeat(shouldRepeat);
            }

            return sounds.first;
        }

        Fancy::fancy.logTime().failure() << "Failed to set repeat-state of sound " << id << " to " << shouldRepeat
                                         << std::endl;
        onError(Enums::ErrorCode::FailedToRepeat);
        return nullptr;
    }
    std::vector<Tab> Window::removeTab(const std::uint32_t &id)
    {
        Globals::gData.removeTabById(id);
        return Globals::gData.getTabs();
    }
    bool Window::stopSound(const std::uint32_t &id)
    {
        auto groupedSounds = this->groupedSounds.write();

        if (groupedSounds->count(id))
        {
            auto sounds = groupedSounds->at(id);

            if (sounds.first)
            {
                sounds.first->stop();
            }
            if (sounds.second)
            {
                sounds.second->stop();
            }

            groupedSounds->erase(id);

            if (groupedSounds->empty())
            {
                onAllSoundsFinished();
            }

            return true;
        }

        return false;
    }
    void Window::stopSounds()
    {
        groupedSounds.write()->clear();
        onAllSoundsFinished();

#if defined(__linux__)
        if (Globals::gAudioBackend)
        {
            if (!Globals::gAudioBackend->stopSoundInput())
            {
                onError(Enums::ErrorCode::FailedToMoveBack);
            }
            if (!Globals::gAudioBackend->stopAllPassthrough())
            {
                onError(Enums::ErrorCode::FailedToMoveBackPassthrough);
            }
        }
#endif
    }
    std::optional<Sound> Window::setCustomLocalVolume(const std::uint32_t &id, const std::optional<int> &localVolume)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            sound->get().localVolume = localVolume;
            auto groupedSounds = this->groupedSounds.write();

            for (const auto &entry : *groupedSounds)
            {
                const auto &local = entry.second.first;
                if (local->getSound().id == sound->get().id)
                {
                    local->setVolume(static_cast<float>(localVolume ? *localVolume : Globals::gSettings.localVolume) /
                                     100.f);
                }
            }

            return *sound;
        }

        Fancy::fancy.logTime().failure() << "Failed to set custom local volume for sound " << id
                                         << ", sound does not exist" << std::endl;
        onError(Enums::ErrorCode::FailedToSetCustomVolume);
        return std::nullopt;
    }
    std::optional<Sound> Window::setCustomRemoteVolume(const std::uint32_t &id, const std::optional<int> &remoteVolume)
    {
        auto sound = Globals::gData.getSound(id);
        auto groupedSounds = this->groupedSounds.write();

        if (sound)
        {
            sound->get().remoteVolume = remoteVolume;

            for (const auto &entry : *groupedSounds)
            {
                const auto &remote = entry.second.second;
                if (remote && remote->getSound().id == sound->get().id && !remote->getPlaybackDevice().isDefault)
                {
                    remote->setVolume(
                        static_cast<float>(remoteVolume ? *remoteVolume : Globals::gSettings.remoteVolume) / 100.f);
                }
            }

            return *sound;
        }

        Fancy::fancy.logTime().failure() << "Failed to set custom remote volume for sound " << id
                                         << ", sound does not exist" << std::endl;
        onError(Enums::ErrorCode::FailedToSetCustomVolume);
        return std::nullopt;
    }
    void Window::onVolumeChanged()
    {
        auto groupedSounds = this->groupedSounds.write();
        for (const auto &entry : *groupedSounds)
        {
            const auto &[local, remote] = entry.second;
            const auto &sound = local->getSound();

            if (local)
            {
                local->setVolume(
                    static_cast<float>(sound.localVolume ? *sound.localVolume : Globals::gSettings.localVolume) /
                    100.f);
            }

            if (remote)
            {
                remote->setVolume(
                    static_cast<float>(sound.remoteVolume ? *sound.remoteVolume : Globals::gSettings.remoteVolume) /
                    100.f);
            }
        }
    }
    Settings Window::changeSettings(Settings settings)
    {
        auto oldSettings = Globals::gSettings;
        Globals::gSettings = settings;

        if ((settings.localVolume != oldSettings.localVolume || settings.remoteVolume != oldSettings.remoteVolume))
        {
            onVolumeChanged();
        }

#if defined(__linux__)
        if (settings.audioBackend != oldSettings.audioBackend)
        {
            stopSounds();

            if (Globals::gAudioBackend)
            {
                Globals::gAudioBackend->destroy();
            }
            Globals::gAudioBackend = AudioBackend::createInstance(settings.audioBackend);
            setDevices();
        }
        if (Globals::gAudioBackend)
        {
            auto groupedSounds = this->groupedSounds.read();
            if (!groupedSounds->empty())
            {
                if (settings.muteDuringPlayback && !oldSettings.muteDuringPlayback)
                {
                    if (!Globals::gAudioBackend->muteInput(true))
                    {
                        onError(Enums::ErrorCode::FailedToMute);
                    }
                }
                else if (!settings.muteDuringPlayback && oldSettings.muteDuringPlayback)
                {
                    if (!Globals::gAudioBackend->muteInput(false))
                    {
                        onError(Enums::ErrorCode::FailedToMute);
                    }
                }
            }
            if (!settings.useAsDefaultDevice && oldSettings.useAsDefaultDevice)
            {
                if (!Globals::gAudioBackend->revertDefault())
                {
                    onError(Enums::ErrorCode::FailedToRevertDefaultSource);
                }
            }
            else if (settings.useAsDefaultDevice && !oldSettings.useAsDefaultDevice)
            {
                Globals::gSettings.outputs.clear();
                if (!Globals::gAudioBackend->stopSoundInput())
                {
                    onError(Enums::ErrorCode::FailedToMoveBack);
                }
                if (!Globals::gAudioBackend->useAsDefault())
                {
                    onError(Enums::ErrorCode::FailedToSetDefaultSource);
                }
            }
            if (settings.outputs != oldSettings.outputs)
            {
                if (!settings.allowMultipleOutputs && settings.outputs.size() > 1)
                {
                    Fancy::fancy.logTime().warning() << "Allow Multiple Outputs is off but got multiple output apps, "
                                                        "falling back to first output in list"
                                                     << std::endl;

                    settings.outputs = {settings.outputs.front()};
                }

                if (!Globals::gAudioBackend->stopSoundInput())
                {
                    onError(Enums::ErrorCode::FailedToMoveBack);
                }

                for (const auto &outputApp : settings.outputs)
                {

                    if (!settings.outputs.empty() && !groupedSounds->empty())
                    {
                        if (!Globals::gAudioBackend->inputSoundTo(Globals::gAudioBackend->getRecordingApp(outputApp)))
                        {
                            onError(Enums::ErrorCode::FailedToMoveToSink);
                        }
                    }
                }
            }
        }
#elif defined(_WIN32)
        if (Globals::gWinSound)
        {
            if (!Globals::gAudio.getPlayingSounds().empty())
            {
                if (settings.muteDuringPlayback && !oldSettings.muteDuringPlayback)
                {
                    if (Globals::gWinSound->getMic())
                    {
                        if (!Globals::gWinSound->getMic()->mute(true))
                        {
                            onError(Enums::ErrorCode::FailedToMute);
                        }
                    }
                }
                else if (!settings.muteDuringPlayback && oldSettings.muteDuringPlayback)
                {
                    if (Globals::gWinSound->getMic())
                    {
                        if (!Globals::gWinSound->getMic()->mute(false))
                        {
                            onError(Enums::ErrorCode::FailedToMute);
                        }
                    }
                }
            }
        }
#endif
        return Globals::gSettings;
    }
    void Window::onHotKeyReceived([[maybe_unused]] const std::vector<Key> &keys)
    {
        Globals::gHotKeys->notify(false);
    }
    std::optional<Tab> Window::refreshTab(const std::uint32_t &id)
    {
        auto tab = Globals::gData.getTab(id);
        if (tab)
        {
            tab->sounds = getTabContent(*tab);
            auto newTab = Globals::gData.setTab(id, *tab);
            if (newTab)
            {
                return newTab;
            }
        }
        Fancy::fancy.logTime().failure() << "Failed to refresh tab " << id << " tab does not exist" << std::endl;
        onError(Enums::ErrorCode::TabDoesNotExist);
        return std::nullopt;
    }
    std::optional<Tab> Window::setSortMode(const std::uint32_t &id, Enums::SortMode sortMode)
    {
        auto tab = Globals::gData.getTab(id);
        if (tab)
        {
            tab->sortMode = sortMode;
            tab->sounds = getTabContent(*tab);
            auto newTab = Globals::gData.setTab(id, *tab);
            if (newTab)
            {
                return newTab;
            }
        }

        Fancy::fancy.logTime().failure() << "Failed to change sortMode for tab " << id << " tab does not exist"
                                         << std::endl;
        onError(Enums::ErrorCode::TabDoesNotExist);
        return std::nullopt;
    }
    std::optional<Sound> Window::setHotkey(const std::uint32_t &id, const std::vector<Key> &hotkeys)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            sound->get().hotkeys = hotkeys;
            return sound->get();
        }
        Fancy::fancy.logTime().failure() << "Failed to set hotkey for sound " << id << ", sound does not exist"
                                         << std::endl;
        onError(Enums::ErrorCode::FailedToSetHotkey);
        return std::nullopt;
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
    std::vector<std::shared_ptr<IconRecordingApp>> Window::getOutputs()
    {
        //* The frontend only uses the stream name and should only show multiple streams that belong to one application
        //* once. The backend (gPulse.getRecordingStreams()) will work with multiple instances, so we need to filter out
        //* duplicates here.

        std::vector<std::shared_ptr<IconRecordingApp>> uniqueStreams;

        if (Globals::gAudioBackend)
        {
            auto streams = Globals::gAudioBackend->getRecordingApps();
            for (auto &stream : streams)
            {
                if (stream->application.find("soundux") != std::string::npos)
                {
                    continue;
                }

                auto item = std::find_if(std::begin(uniqueStreams), std::end(uniqueStreams),
                                         [&](const auto &_stream) { return stream->name == _stream->name; });
                if (stream && item == std::end(uniqueStreams))
                {
                    auto iconStream = std::make_shared<IconRecordingApp>(*stream);
                    if (Globals::gIcons)
                    {
                        if (auto pulseApp = std::dynamic_pointer_cast<PulseRecordingApp>(stream); pulseApp)
                        {
                            auto icon = Soundux::Globals::gIcons->getIcon(static_cast<int>(pulseApp->pid));
                            if (icon)
                            {
                                iconStream->appIcon = *icon;
                            }
                        }
                        else if (auto pipeWireApp = std::dynamic_pointer_cast<PipeWireRecordingApp>(stream);
                                 pipeWireApp)
                        {
                            auto icon = Soundux::Globals::gIcons->getIcon(static_cast<int>(pipeWireApp->pid));
                            if (icon)
                            {
                                iconStream->appIcon = *icon;
                            }
                        }
                    }

                    uniqueStreams.emplace_back(iconStream);
                }
            }
        }

        return uniqueStreams;
    }
    std::vector<std::shared_ptr<IconPlaybackApp>> Window::getPlayback()
    {
        std::vector<std::shared_ptr<IconPlaybackApp>> uniqueStreams;

        if (Globals::gAudioBackend)
        {
            auto streams = Globals::gAudioBackend->getPlaybackApps();

            for (auto &stream : streams)
            {
                if (stream->application.find("soundux") != std::string::npos)
                {
                    continue;
                }

                auto item = std::find_if(std::begin(uniqueStreams), std::end(uniqueStreams),
                                         [&](const auto &_stream) { return stream->name == _stream->name; });
                if (stream && item == std::end(uniqueStreams))
                {
                    auto iconStream = std::make_shared<IconPlaybackApp>(*stream);

                    if (Globals::gIcons)
                    {
                        if (auto pulseApp = std::dynamic_pointer_cast<PulsePlaybackApp>(stream); pulseApp)
                        {
                            auto icon = Soundux::Globals::gIcons->getIcon(static_cast<int>(pulseApp->pid));
                            if (icon)
                            {
                                iconStream->appIcon = *icon;
                            }
                        }
                        if (auto pipeWireApp = std::dynamic_pointer_cast<PipeWirePlaybackApp>(stream); pipeWireApp)
                        {
                            auto icon = Soundux::Globals::gIcons->getIcon(static_cast<int>(pipeWireApp->pid));
                            if (icon)
                            {
                                iconStream->appIcon = *icon;
                            }
                        }
                    }

                    uniqueStreams.emplace_back(iconStream);
                }
            }
        }

        return uniqueStreams;
    }
    bool Window::startPassthrough(const std::string &name)
    {
        bool success = true;
        if (Globals::gAudioBackend && !Globals::gSettings.outputs.empty())
        {
            for (const auto &outputApp : Globals::gSettings.outputs)
            {
                if (!Globals::gAudioBackend->inputSoundTo(Globals::gAudioBackend->getRecordingApp(outputApp)))
                {
                    onError(Enums::ErrorCode::FailedToMoveToSink);
                    success = false;
                }
            }

            if (success)
            {
                if (!Globals::gAudioBackend->passthroughFrom(Globals::gAudioBackend->getPlaybackApp(name)))
                {
                    success = false;
                }
            }

            if (!success)
            {
                onError(Enums::ErrorCode::FailedToStartPassthrough);
            }
        }

        return success;
    }
    void Window::stopPassthrough(const std::string &name)
    {
        if (Globals::gAudioBackend)
        {
            if (!Globals::gAudioBackend->stopPassthrough(name))
            {
                onError(Enums::ErrorCode::FailedToMoveBackPassthrough);
            }

            auto groupedSounds = this->groupedSounds.read();
            if (groupedSounds->empty() && Globals::gAudioBackend->currentlyPassedThrough().empty())
            {
                if (!Globals::gAudioBackend->stopSoundInput())
                {
                    onError(Enums::ErrorCode::FailedToMoveBack);
                }
            }
        }
    }
#else
    std::vector<AudioDevice> Window::getOutputs()
    {
        return Globals::gAudio.getAudioDevices();
    }
#endif
    void Window::onSoundFinished(const std::shared_ptr<PlayingSound> &sound)
    {
        auto groupedSounds = this->groupedSounds.write();
        if (groupedSounds->count(sound->getId()))
        {
            groupedSounds->erase(sound->getId());
        }

        if (groupedSounds->empty())
        {
            onAllSoundsFinished();
        }
    }
    void Window::onAllSoundsFinished()
    {
        if (!Globals::gSettings.pushToTalkKeys.empty())
        {
            Globals::gHotKeys->releaseKeys(Globals::gSettings.pushToTalkKeys);
        }

#if defined(__linux__)
        if (Globals::gAudioBackend)
        {
            if (Globals::gSettings.muteDuringPlayback)
            {
                if (!Globals::gAudioBackend->muteInput(false))
                {
                    onError(Enums::ErrorCode::FailedToMute);
                }
            }
            if (Globals::gAudioBackend->currentlyPassedThrough().empty())
            {
                if (!Globals::gAudioBackend->stopSoundInput())
                {
                    onError(Enums::ErrorCode::FailedToMoveBack);
                }
            }
        }
#elif defined(_WIN32)
        if (Globals::gSettings.muteDuringPlayback)
        {
            if (Globals::gWinSound && Globals::gWinSound->getMic())
            {
                if (!Globals::gWinSound->getMic()->mute(false))
                {
                    onError(Enums::ErrorCode::FailedToMute);
                }
            }
        }
#endif
    }
    void Window::onSoundPlayed([[maybe_unused]] const std::shared_ptr<PlayingSound> &sound)
    {
        if (!Globals::gSettings.pushToTalkKeys.empty())
        {
            Globals::gHotKeys->pressKeys(Globals::gSettings.pushToTalkKeys);
        }
    }
    void Window::setIsOnFavorites(bool state)
    {
        Globals::gData.isOnFavorites = state;
    }
    bool Window::deleteSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            if (!Helpers::deleteFile(sound->get().path, Globals::gSettings.deleteToTrash))
            {
                onError(Enums::ErrorCode::FailedToDelete);
                return false;
            }
            return true;
        }

        Fancy::fancy.logTime().failure() << "Sound " << id << " not found" << std::endl;
        onError(Enums::ErrorCode::SoundNotFound);
        return false;
    }
    bool Window::toggleSoundPlayback()
    {
        bool shouldPause = true;
        auto groupedSounds = this->groupedSounds.read();

        for (const auto &sounds : *groupedSounds)
        {
            const auto &[local, remote] = sounds.second;
            if (local->isPaused())
            {
                shouldPause = false;
                break;
            }
        }

        for (const auto &sounds : *groupedSounds)
        {
            const auto &[local, remote] = sounds.second;

            if (shouldPause)
            {
                local->pause();
                if (remote)
                {
                    remote->pause();
                }
            }
            else
            {
                local->resume();
                if (remote)
                {
                    remote->resume();
                }
            }
        }

        return shouldPause;
    }
#if defined(__linux__)
    IconRecordingApp::IconRecordingApp(const RecordingApp &base)
    {
        name = base.name;
        application = base.application;
    }
    IconPlaybackApp::IconPlaybackApp(const PlaybackApp &base)
    {
        name = base.name;
        application = base.application;
    }
#endif
} // namespace Soundux::Objects