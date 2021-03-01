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

            if (file.extension() != ".mp3" && file.extension() != ".wav" && file.extension() != ".flac")
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
            sound.id = ++Globals::gData.soundIdCounter;

            if (auto oldSound = std::find_if(tab.sounds.begin(), tab.sounds.end(),
                                             [&sound](const auto &item) { return item.path == sound.path; });
                oldSound != tab.sounds.end())
            {
                sound.hotkeys = oldSound->hotkeys;
            }

            rtn.emplace_back(sound);
        }

        std::sort(rtn.begin(), rtn.end(),
                  [](const auto &first, const auto &second) { return first.modifiedDate > second.modifiedDate; });

        return rtn;
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
            Fancy::fancy.logTime().failure() << "Selected Folder does not exist!" << std::endl;
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
            if (Globals::gPulse.moveApplicationToSinkMonitor(Globals::gSettings.output))
            {
                if (!Globals::gSettings.allowOverlapping)
                {
                    Globals::gAudio.stopAll();
                }
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
        Fancy::fancy.logTime().failure() << "Failed to play sound " << id << std::endl;
        onError(ErrorCode::FailedToPlay);
        return std::nullopt;
    }
#else
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        auto device = Globals::gAudio.getAudioDevice(Globals::gSettings.output);

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
        Fancy::fancy.logTime().failure() << "Failed to play sound " << id << std::endl;
        onError(ErrorCode::FailedToPlay);
        return std::nullopt;
    }
#endif
    std::optional<PlayingSound> Window::pauseSound(const std::uint32_t &id)
    {
        std::shared_lock lock(groupedSoundsMutex);
        if (groupedSounds.find(id) == groupedSounds.end())
        {
            Fancy::fancy.logTime().failure() << "Failed to find remoteSound of sound " << id << std::endl;
            return std::nullopt;
        }

        auto playingSound = Globals::gAudio.pause(id);
        auto remotePlayingSound = Globals::gAudio.pause(groupedSounds.at(id));

        if (playingSound && remotePlayingSound)
        {
            return *playingSound;
        }

        Fancy::fancy.logTime().failure() << "Failed to pause sound " << id << std::endl;
        onError(ErrorCode::FailedToPause);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::resumeSound(const std::uint32_t &id)
    {
        std::shared_lock lock(groupedSoundsMutex);
        if (groupedSounds.find(id) == groupedSounds.end())
        {
            Fancy::fancy.logTime().failure() << "Failed to find remoteSound of sound " << id << std::endl;
            return std::nullopt;
        }

        auto playingSound = Globals::gAudio.resume(id);
        auto remotePlayingSound = Globals::gAudio.resume(groupedSounds.at(id));

        if (playingSound && remotePlayingSound)
        {
            return *playingSound;
        }

        Fancy::fancy.logTime().failure() << "Failed to resume sound " << id << std::endl;
        onError(ErrorCode::FailedToResume);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::seekSound(const std::uint32_t &id, std::uint64_t seekTo)
    {
        std::shared_lock lock(groupedSoundsMutex);
        if (groupedSounds.find(id) == groupedSounds.end())
        {
            Fancy::fancy.logTime().failure() << "Failed to find remoteSound of sound " << id << std::endl;
            return std::nullopt;
        }

        auto playingSound = Globals::gAudio.seek(id, seekTo);
        auto remotePlayingSound = Globals::gAudio.seek(groupedSounds.at(id), seekTo);
        if (playingSound && remotePlayingSound)
        {
            return *playingSound;
        }

        Fancy::fancy.logTime().failure() << "Failed to seek sound " << id << " to " << seekTo << std::endl;
        onError(ErrorCode::FailedToSeek);
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::repeatSound(const std::uint32_t &id, bool shouldRepeat)
    {
        std::shared_lock lock(groupedSoundsMutex);
        if (groupedSounds.find(id) == groupedSounds.end())
        {
            Fancy::fancy.logTime().failure() << "Failed to find remoteSound of sound " << id << std::endl;
            return std::nullopt;
        }

        auto playingSound = Globals::gAudio.repeat(id, shouldRepeat);
        auto remotePlayingSound = Globals::gAudio.repeat(groupedSounds.at(id), shouldRepeat);
        if (playingSound && remotePlayingSound)
        {
            return *playingSound;
        }

        Fancy::fancy.logTime().failure() << "Failed to set repeatstate of sound " << id << " to " << shouldRepeat
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
        std::shared_lock lock(groupedSoundsMutex);
        if (groupedSounds.find(id) == groupedSounds.end())
        {
            Fancy::fancy.logTime().failure() << "Failed to find remoteSound of sound " << id << std::endl;
            return false;
        }
        auto remoteId = groupedSounds.at(id);
        lock.unlock();

        auto status = Globals::gAudio.stop(id);
        auto remoteStatus = Globals::gAudio.stop(remoteId);

#if defined(__linux__)
        if (Globals::gAudio.getPlayingSounds().empty())
        {
            if (!Globals::gPulse.moveBackCurrentApplication())
            {
                Fancy::fancy.logTime().failure()
                    << "Failed to move back current application, sound: " << id << std::endl;
                onError(ErrorCode::FailedToMoveBack);
            }
        }
#endif

        return status && remoteStatus;
    }
    void Window::stopSounds()
    {
        Globals::gQueue.push_unique(0, []() { Globals::gAudio.stopAll(); });
#if defined(__linux__)
        if (!Globals::gPulse.moveBackCurrentApplication())
        {
            Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
            onError(ErrorCode::FailedToMoveBack);
        }
        if (!Globals::gPulse.moveBackApplicationFromPassthrough())
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
            if (!Globals::gPulse.setDefaultSourceToSoundboardSink())
            {
                Fancy::fancy.logTime().failure() << "Failed to set default source" << std::endl;
                onError(ErrorCode::FailedToSetDefaultSource);
            }
        }
#endif
        Globals::gSettings = settings;
    }
    void Window::onHotKeyReceived([[maybe_unused]] const std::vector<int> &keys)
    {
        Globals::gHotKeys.shouldNotify(false);
    }
    void Window::onEvent(const std::function<void()> &function)
    {
        std::unique_lock lock(eventMutex);
        eventQueue.emplace(function);
        lock.unlock();

        shouldCheck = true;
    }
    void Window::progressEvents()
    {
        if (shouldCheck)
        {
            std::shared_lock lock(eventMutex);
            if (!eventQueue.empty())
            {
                lock.unlock();
                {
                    std::unique_lock uLock(eventMutex);
                    while (!eventQueue.empty())
                    {
                        auto front = std::move(eventQueue.front());
                        eventQueue.pop();

                        uLock.unlock();
                        front();
                        uLock.lock();
                    }
                }
                lock.lock();
            }
        }
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
#if defined(__linux__)
    std::vector<PulseRecordingStream> Window::getOutput()
    {
        Globals::gPulse.refreshRecordingStreams();
        return Globals::gPulse.getRecordingStreams();
    }
    std::vector<PulsePlaybackStream> Window::getPlayback()
    {
        Globals::gPulse.refreshPlaybackStreams();
        return Globals::gPulse.getPlaybackStreams();
    }
    std::optional<PulsePlaybackStream> Window::startPassthrough(const std::string &name)
    {
        if (Globals::gPulse.moveApplicationToSinkMonitor(Globals::gSettings.output))
        {
            return Globals::gPulse.moveApplicationToApplicationPassthrough(name);
        }
        Fancy::fancy.logTime().failure() << "Failed to start passthrough for application: " << name << std::endl;
        onError(ErrorCode::FailedToStartPassthrough);
        return std::nullopt;
    }
    void Window::stopPassthrough()
    {
        if (Globals::gAudio.getPlayingSounds().empty())
        {
            if (!Globals::gPulse.moveBackCurrentApplication())
            {
                Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                onError(ErrorCode::FailedToMoveBack);
            }
        }
        if (!Globals::gPulse.moveBackApplicationFromPassthrough())
        {
            Fancy::fancy.logTime().failure() << "Failed to move back current passthrough application" << std::endl;
            onError(ErrorCode::FailedToMoveBackPassthrough);
        }
    }
#else
    std::vector<AudioDevice> Window::getOutput()
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

#if defined(__linux__)
        if (Globals::gAudio.getPlayingSounds().size() == 1 && !Globals::gPulse.currentlyPassingthrough())
        {
            if (!Globals::gPulse.moveBackCurrentApplication())
            {
                Fancy::fancy.logTime().failure() << "Failed to move back current application" << std::endl;
                onError(ErrorCode::FailedToMoveBack);
            }
        }
#endif
    }
} // namespace Soundux::Objects