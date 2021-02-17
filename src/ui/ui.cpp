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
        // TODO(curve): Save config
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

            sound.name = file.filename().u8string();
            sound.path = file.u8string();

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
    std::optional<PlayingSound> Window::playSound(const std::uint32_t &id)
    {
        auto sound = Globals::gData.getSound(id);
        if (sound)
        {
            // TODO(curve): PlayingDevice
            auto playingSound = Globals::gAudio.play(*sound);
            if (playingSound)
            {
                return *playingSound;
            }
        }
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::pauseSound(const std::uint32_t &id)
    {
        auto playingSound = Globals::gAudio.pause(id);
        if (playingSound)
        {
            return *playingSound;
        }
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::resumeSound(const std::uint32_t &id)
    {
        auto playingSound = Globals::gAudio.resume(id);
        if (playingSound)
        {
            return *playingSound;
        }
        return std::nullopt;
    }
    std::optional<PlayingSound> Window::seekSound(const std::uint32_t &id, std::uint64_t seekTo)
    {
        auto playingSound = Globals::gAudio.seek(id, seekTo);
        if (playingSound)
        {
            return *playingSound;
        }
        return std::nullopt;
    }
    void Window::stopSound(const std::uint32_t &id)
    {
        Globals::gAudio.stop(id);
    }
    void Window::stopSounds()
    {
        Globals::gAudio.stopAll();
    }
    void Window::changeSettings(const Settings &settings)
    {
        Globals::gSettings = settings;
        // TODO(curve): Override existing Config Properties
    }
    void Window::onHotKeyReceived([[maybe_unused]] const std::vector<std::string> &keys)
    {
        Globals::gHotKeys.shouldNotify(false);
    }
    void Window::onEvent(const std::function<void()> &function)
    {
        std::unique_lock lock(eventMutex);
        eventQueue.push(function);
    }
    void Window::progressEvents()
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
} // namespace Soundux::Objects