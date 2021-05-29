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
        Globals::gHotKeys.init();
        for (auto &tab : Globals::gData.getTabs())
        {
            tab.sounds = getTabContent(tab);
            Globals::gData.setTab(tab.id, tab);
        }
    }
    Window::~Window()
    {
        NFD::Quit();
        Globals::gHotKeys.stop();
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
                std::sort(rtn.begin(), rtn.end(), [](const auto &first, const auto &second) {
                    return first.modifiedDate > second.modifiedDate;
                });
                break;
            case Enums::SortMode::Alphabetical_Ascending:
                std::sort(rtn.begin(), rtn.end(), [](const auto &first, const auto &second) {
                    return first.modifiedDate < second.modifiedDate;
                });
                break;
            }

            return rtn;
        }

        Fancy::fancy.logTime().warning() << "Path " >> tab.path << " does not exist" << std::endl;
        return {};
    }
    std::vector<Tab> Window::addTab()
    {
        nfdnchar_t *outpath = {};
        auto result = NFD::PickFolder(outpath, nullptr);

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

                if (!Globals::gData.doesTabExist(rootPath))
                {
                    Tab rootTab;
                    rootTab.path = rootPath;
                    rootTab.sounds = getTabContent(rootTab);
                    rootTab.name = std::filesystem::path(rootPath).filename().u8string();

                    tabs.emplace_back(Globals::gData.addTab(std::move(rootTab)));
                }

                for (const auto &entry : std::filesystem::directory_iterator(rootPath))
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
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            if (!Globals::gSettings.allowOverlapping)
            {
                stopSounds(true);
            }
            if (Globals::gSettings.muteDuringPlayback)
            {
                if (Globals::gAudioBackend)
                {
                    Globals::gAudioBackend->muteInput(true);
                }
            }
            if (!Globals::gSettings.pushToTalkKeys.empty())
            {
                Globals::gHotKeys.pressKeys(Globals::gSettings.pushToTalkKeys);
            }

            auto playingSound = Globals::gAudio.play(*sound);
            auto remotePlayingSound = Globals::gAudio.play(*sound, Globals::gAudio.nullSink);

            if (playingSound && remotePlayingSound)
            {
                groupedSounds->insert({playingSound->id, remotePlayingSound->id});
                if (Globals::gSettings.outputs.empty() && playingSound)
                {
                    return *playingSound;
                }
                if (!Globals::gSettings.outputs.empty() && Globals::gAudioBackend)
                {
                    bool moveSuccess = false;
                    for (const auto &outputApp : Globals::gSettings.outputs)
                    {
                        if (Globals::gAudioBackend->inputSoundTo(Globals::gAudioBackend->getRecordingApp(outputApp)))
                        {
                            moveSuccess = true;
                        }
                        else
                        {
                            Fancy::fancy.logTime().failure() << "Failed to move Application " << outputApp
                                                             << " to soundux sink for sound " << id << std::endl;
                            onError(Enums::ErrorCode::FailedToMoveToSink);
                        }
                    }

                    if (!moveSuccess)
                    {
                        if (playingSound)
                            stopSound(playingSound->id);
                        if (remotePlayingSound)
                            stopSound(remotePlayingSound->id);

                        Fancy::fancy.logTime().failure() << "Failed to move Applications " << Globals::gSettings.outputs
                                                         << " to soundux sink for sound " << id << std::endl;
                        onError(Enums::ErrorCode::FailedToMoveToSink);
                        return std::nullopt;
                    }

                    return *playingSound;
                }
            }
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Sound " << id << " not found" << std::endl;
            onError(Enums::ErrorCode::SoundNotFound);
            return std::nullopt;
        }

        Fancy::fancy.logTime().failure() << "Failed to play sound " << id << std::endl;
        onError(Enums::ErrorCode::FailedToPlay);
        return std::nullopt;
    }
#else
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            if (!Globals::gSettings.allowOverlapping)
            {
                stopSounds();
            }
            if (!Globals::gSettings.pushToTalkKeys.empty())
            {
                Globals::gHotKeys.pressKeys(Globals::gSettings.pushToTalkKeys);
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
                {
                    stopSound(playingSound->id);
                }
                if (remotePlayingSound)
                {
                    stopSound(remotePlayingSound->id);
                }

                return std::nullopt;
            }

            return *playingSound;
        }
        else
        {
            Fancy::fancy.logTime().failure() << "Sound " << id << " not found" << std::endl;
            onError(Enums::ErrorCode::SoundNotFound);
            return std::nullopt;
        }

        Fancy::fancy.logTime().failure() << "Failed to play sound " << id << std::endl;
        onError(Enums::ErrorCode::FailedToPlay);
        return std::nullopt;
    }
#endif
    std::optional<PlayingSound> Window::pauseSound(const std::uint32_t &id)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.outputs.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            auto scoped = groupedSounds.scoped();
            if (scoped->find(id) == scoped->end())
            {
                if (!Globals::gSettings.outputs.empty() || !Globals::gSettings.useAsDefaultDevice)
                {
                    Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                }
            }
            else
            {
                remoteSoundId = scoped->at(id);
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
        onError(Enums::ErrorCode::FailedToPause);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::resumeSound(const std::uint32_t &id)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.outputs.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            auto scoped = groupedSounds.scoped();
            if (scoped->find(id) == scoped->end())
            {
                if (!Globals::gSettings.outputs.empty() || !Globals::gSettings.useAsDefaultDevice)
                {
                    Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                }
            }
            else
            {
                remoteSoundId = scoped->at(id);
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
        onError(Enums::ErrorCode::FailedToResume);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::seekSound(const std::uint32_t &id, std::uint64_t seekTo)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.outputs.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            auto scoped = groupedSounds.scoped();
            if (scoped->find(id) == scoped->end())
            {
                if (!Globals::gSettings.outputs.empty() || !Globals::gSettings.useAsDefaultDevice)
                {
                    Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                }
            }
            else
            {
                remoteSoundId = scoped->at(id);
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
        onError(Enums::ErrorCode::FailedToSeek);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::repeatSound(const std::uint32_t &id, bool shouldRepeat)
    {
        std::optional<std::uint32_t> remoteSoundId;
        if (!Globals::gSettings.outputs.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            auto scoped = groupedSounds.scoped();
            if (scoped->find(id) == scoped->end())
            {
                if (!Globals::gSettings.outputs.empty() || !Globals::gSettings.useAsDefaultDevice)
                {
                    Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                }
            }
            else
            {
                remoteSoundId = scoped->at(id);
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
        onError(Enums::ErrorCode::FailedToRepeat);
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
        if (!Globals::gSettings.outputs.empty() && !Globals::gSettings.useAsDefaultDevice)
        {
            auto scoped = groupedSounds.scoped();
            if (scoped->find(id) == scoped->end())
            {
                Fancy::fancy.logTime().warning() << "Failed to find remoteSound of sound " << id << std::endl;
                return false;
            }

            remoteSoundId = scoped->at(id);
        }

        auto status = Globals::gAudio.stop(id);
        if (remoteSoundId)
        {
            Globals::gAudio.stop(*remoteSoundId);
        }

        if (Globals::gAudio.getPlayingSounds().empty())
        {
            onAllSoundsFinished();
        }

        return status;
    }
    void Window::stopSounds(bool sync)
    {
        if (!sync)
        {
            Globals::gQueue.push_unique(0, []() { Globals::gAudio.stopAll(); });
        }
        else
        {
            Globals::gAudio.stopAll();
        }
        onAllSoundsFinished();

#if defined(__linux__)
        if (Globals::gAudioBackend)
        {
            if (!Globals::gAudioBackend->stopSoundInput())
            {
                Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                onError(Enums::ErrorCode::FailedToMoveBack);
            }
            if (!Globals::gAudioBackend->stopAllPassthrough())
            {
                Fancy::fancy.logTime().failure() << "Failed to move back current passthrough applications" << std::endl;
                onError(Enums::ErrorCode::FailedToMoveBackPassthrough);
            }
        }
#endif
    }
    Settings Window::changeSettings(Settings settings)
    {
        auto oldSettings = Globals::gSettings;
        Globals::gSettings = settings;

        if (!Globals::gAudio.getPlayingSounds().empty())
        {
            for (const auto &sound : Globals::gAudio.getPlayingSounds())
            {
                if (sound.playbackDevice.isDefault)
                {
                    sound.raw.device.load()->masterVolumeFactor =
                        static_cast<float>(Globals::gSettings.localVolume) / 100.f;
                }
                else
                {
                    sound.raw.device.load()->masterVolumeFactor =
                        static_cast<float>(Globals::gSettings.remoteVolume) / 100.f;
                }
            }
        }

#if defined(__linux__)
        if (settings.audioBackend != oldSettings.audioBackend)
        {
            stopSounds(true);

            if (Globals::gAudioBackend)
            {
                Globals::gAudioBackend->destroy();
            }

            Globals::gAudioBackend = AudioBackend::createInstance(settings.audioBackend);
            Globals::gAudio.setup();
        }
        if (Globals::gAudioBackend)
        {
            if (!Globals::gAudio.getPlayingSounds().empty())
            {
                if (settings.muteDuringPlayback && !oldSettings.muteDuringPlayback)
                {
                    if (!Globals::gAudioBackend->muteInput(true))
                    {
                        Fancy::fancy.logTime().failure() << "Failed to mute input" << std::endl;
                    }
                }
                else if (!settings.muteDuringPlayback && oldSettings.muteDuringPlayback)
                {
                    if (!Globals::gAudioBackend->muteInput(false))
                    {
                        Fancy::fancy.logTime().failure() << "Failed to un-mute input" << std::endl;
                    }
                }
            }
            if (!settings.useAsDefaultDevice && oldSettings.useAsDefaultDevice)
            {
                if (!Globals::gAudioBackend->revertDefault())
                {
                    Fancy::fancy.logTime().failure() << "Failed to move back default source" << std::endl;
                    onError(Enums::ErrorCode::FailedToRevertDefaultSource);
                }
            }
            else if (settings.useAsDefaultDevice && !oldSettings.useAsDefaultDevice)
            {
                Globals::gSettings.outputs.clear();
                if (!Globals::gAudioBackend->stopSoundInput())
                {
                    Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
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
                    Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                    onError(Enums::ErrorCode::FailedToMoveBack);
                }

                for (const auto &outputApp : settings.outputs)
                {
                    if (!settings.outputs.empty() && !Globals::gAudio.getPlayingSounds().empty())
                    {
                        Globals::gAudioBackend->inputSoundTo(Globals::gAudioBackend->getRecordingApp(outputApp));
                    }
                }
            }
        }
#endif
        return Globals::gSettings;
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
                    success = false;
                }
            }

            if (success)
            {
                if (!Globals::gAudioBackend->passthroughFrom(Globals::gAudioBackend->getPlaybackApp(name)))
                {
                    Fancy::fancy.logTime().failure()
                        << "Failed to move application: " << name << " to passthrough" << std::endl;
                    success = false;
                }
            }
            else
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
            if (Globals::gAudio.getPlayingSounds().empty() &&
                Globals::gAudioBackend->currentlyPassedThrough().size() == 1)
            {
                if (!Globals::gAudioBackend->stopSoundInput())
                {
                    Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                    onError(Enums::ErrorCode::FailedToMoveBack);
                }
            }

            if (!Globals::gAudioBackend->stopPassthrough(name))
            {
                Fancy::fancy.logTime().failure() << "Failed to move back current passthrough application" << std::endl;
                onError(Enums::ErrorCode::FailedToMoveBackPassthrough);
            }
        }
    }
#else
    std::vector<AudioDevice> Window::getOutputs()
    {
        return Globals::gAudio.getAudioDevices();
    }
#endif
    void Window::onSoundFinished(const PlayingSound &sound)
    {
        auto scoped = groupedSounds.scoped();
        if (scoped->find(sound.id) != scoped->end())
        {
            scoped->erase(sound.id);
        }
        scoped.unlock();

        if (Globals::gAudio.getPlayingSounds().size() == 1)
        {
            onAllSoundsFinished();
        }
    }
    void Window::onAllSoundsFinished()
    {
        if (!Globals::gSettings.pushToTalkKeys.empty())
        {
            Globals::gHotKeys.releaseKeys(Globals::gSettings.pushToTalkKeys);
        }

#if defined(__linux__)
        if (Globals::gAudioBackend)
        {
            if (Globals::gSettings.muteDuringPlayback)
            {
                Globals::gAudioBackend->muteInput(false);
            }
            if (Globals::gAudioBackend->currentlyPassedThrough().empty())
            {
                if (!Globals::gAudioBackend->stopSoundInput())
                {
                    Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                    onError(Enums::ErrorCode::FailedToMoveBack);
                }
            }
        }
#endif
    }
    void Window::onSoundPlayed([[maybe_unused]] const PlayingSound &sound)
    {
        if (!Globals::gSettings.pushToTalkKeys.empty())
        {
            Globals::gHotKeys.pressKeys(Globals::gSettings.pushToTalkKeys);
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