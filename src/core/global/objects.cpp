#include "objects.hpp"
#include "globals.hpp"
#include <algorithm>
#include <fancy.hpp>
#include <functional>
#include <optional>

namespace Soundux::Objects
{
    Sound::Sound() : id(Globals::gData.soundIdCounter++) {}

    void Data::addTab(Tab tab)
    {
        tab.id = tabs.size();
        tabs.push_back(tab);
        std::unique_lock lock(Globals::gSoundsMutex);

        for (auto &sound : tabs.back().sounds)
        {
            Globals::gSounds.insert({sound.id, sound});
        }
    }
    void Data::removeTabById(const std::uint32_t &index)
    {
        std::unique_lock lock(Globals::gSoundsMutex);
        if (tabs.size() > index)
        {
            auto &tab = tabs.at(index);
            for (auto &sound : tab.sounds)
            {
                Globals::gSounds.erase(sound.id);
            }

            tabs.erase(tabs.begin() + index);
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Tried to remove non existant tab" << std::endl;
        }
    }
    void Data::setTabs(const std::vector<Tab> &newTabs)
    {
        tabs = newTabs;
        std::unique_lock lock(Globals::gSoundsMutex);

        Globals::gSounds.clear();
        for (auto &tab : tabs)
        {
            for (auto &sound : tab.sounds)
            {
                Globals::gSounds.insert({sound.id, sound});
            }
        }
    }
    std::vector<Tab> Data::getTabs() const
    {
        return tabs;
    }
    std::optional<std::reference_wrapper<Sound>> Data::getSound(const std::uint32_t &id)
    {
        std::shared_lock lock(Globals::gSoundsMutex);

        if (Globals::gSounds.find(id) != Globals::gSounds.end())
        {
            return Globals::gSounds.at(id);
        }

        Fancy::fancy.logTime().warning() << "Tried to access non existant sound " << id << std::endl;
        return std::nullopt;
    }
} // namespace Soundux::Objects