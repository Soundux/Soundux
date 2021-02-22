#include "ui.hpp"
#include "../core/global/globals.hpp"
#include <cstdint>
#include <fancy.hpp>
#include <filesystem>
#include <nfd.hpp>
#include <optional>

namespace Soundux::Objects
{
    void Window::setup()
    {
        NFD::Init();
        Globals::gHotKeys.init();
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
            sound.name = file.stem().u8string();
            sound.id = ++Globals::gData.soundIdCounter;

            if (auto oldSound = std::find_if(tab.sounds.begin(), tab.sounds.end(),
                                             [&sound](const auto &item) { return item.path == sound.path; });
                oldSound != tab.sounds.end())
            {
                sound.hotkeys = oldSound->hotkeys;
            }

            rtn.push_back(sound);
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
            std::string path(outpath);
            NFD_FreePathN(outpath);

            if (std::filesystem::exists(path))
            {
                Tab tab;
                tab.path = path;
                tab.sounds = refreshTabSounds(tab);
                tab.name = std::filesystem::path(path).filename();

                tab = Globals::gData.addTab(std::move(tab));

                return tab;
            }
            Fancy::fancy.logTime().failure() << "Selected Folder does not exist!" << std::endl;
        }
        return std::nullopt;
    }
#if defined(__linux__)
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id, const std::string &applicationName)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            if (Globals::gPulse.moveApplicationToSinkMonitor(applicationName))
            {
                auto playingSound = Globals::gAudio.play(*sound);
                auto remotePlayingSound = Globals::gAudio.play(*sound, *Globals::gAudio.sinkAudioDevice, true);

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
        }
        return std::nullopt;
    }
#else
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id, const std::string &deviceName)
    {
        auto sound = Globals::gData.getSound(id);
        auto device = Globals::gAudio.getAudioDevice(deviceName);

        if (sound && device)
        {
            auto playingSound = Globals::gAudio.play(*sound);
            auto remotePlayingSound = Globals::gAudio.play(*sound, *device, true);

            if (playingSound && remotePlayingSound)
            {
                return *playingSound;
            }

            if (playingSound)
                stopSound(playingSound->id);
            if (remotePlayingSound)
                stopSound(remotePlayingSound->id);
        }
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

        auto status = Globals::gAudio.stop(id);
        auto remoteStatus = Globals::gAudio.stop(groupedSounds.at(id));

#if defined(__linux__)
        if (Globals::gAudio.getPlayingSounds().empty())
        {
            Globals::gPulse.moveBackCurrentApplication();
        }
#endif

        return status && remoteStatus;
    }
    void Window::stopSounds()
    {
        Globals::gQueue.push_unique(0, []() { Globals::gAudio.stopAll(); });
#if defined(__linux__)
        Globals::gPulse.moveBackCurrentApplication();
        Globals::gPulse.moveBackApplicationFromPassthrough();
#endif
    }
    void Window::changeSettings(const Settings &settings)
    {
        Globals::gSettings = settings;
    }
    void Window::onHotKeyReceived([[maybe_unused]] const std::vector<int> &keys)
    {
        Globals::gHotKeys.shouldNotify(false);
    }
    void Window::onEvent(const std::function<void()> &function)
    {
        std::unique_lock lock(eventMutex);
        eventQueue.push(function);
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
        return std::nullopt;
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
    std::optional<PulsePlaybackStream> Window::startPassthrough(const std::string &name, const std::string &output)
    {
        if (Globals::gPulse.moveApplicationToSinkMonitor(output))
        {
            return Globals::gPulse.moveApplicationToApplicationPassthrough(name);
        }
        return std::nullopt;
    }
    void Window::stopPassthrough()
    {
        if (Globals::gAudio.getPlayingSounds().empty())
        {
            Globals::gPulse.moveBackCurrentApplication();
        }
        Globals::gPulse.moveBackApplicationFromPassthrough();
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
        if (Globals::gAudio.getPlayingSounds().size() == 1)
        {
            Globals::gPulse.moveBackCurrentApplication();
        }
#endif
    }
} // namespace Soundux::Objects