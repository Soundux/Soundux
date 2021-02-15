#include "ui.hpp"
#include "../core/global/globals.hpp"
#include <fancy.hpp>
#include <filesystem>
#include <nfd.hpp>

namespace Soundux::Objects
{
    void Window::setup()
    {
        NFD::Init();
    }
    Window::~Window()
    {
        NFD::Quit();
    }
    std::vector<Sound> Window::getTabSounds(const Tab &tab) const
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

        return rtn;
    }
    void Window::addTab()
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
                tab.sounds = getTabSounds(tab);
                tab.name = std::filesystem::path(path).filename();

                Globals::gData.addTab(std::move(tab));
            }
            else
            {
                Fancy::fancy.logTime().failure() << "Selected Folder does not exist!" << std::endl;
            }
        }
    }
} // namespace Soundux::Objects