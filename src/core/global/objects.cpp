#include "objects.hpp"
#include "globals.hpp"
#include <algorithm>
#include <fancy.hpp>
#include <functional>
#include <optional>

namespace Soundux::Objects
{
    Tab Data::addTab(Tab tab)
    {
        tab.id = tabs.size();
        tabs.emplace_back(tab);
        std::unique_lock lock(Globals::gSoundsMutex);

        for (auto &sound : tabs.back().sounds)
        {
            Globals::gSounds.insert({sound.id, sound});
        }

        return tabs.back();
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

            for (std::size_t i = 0; tabs.size() > i; i++)
            {
                tabs.at(i).id = i;
            }
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
        for (std::size_t i = 0; tabs.size() > i; i++)
        {
            auto &tab = tabs.at(i);
            tab.id = i;

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
    std::optional<Tab> Data::getTab(const std::uint32_t &id) const
    {
        if (tabs.size() > id)
        {
            return tabs.at(id);
        }

        Fancy::fancy.logTime().warning() << "Tried to access non existant tab " << id << std::endl;
        return std::nullopt;
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
    std::optional<Tab> Data::setTab(const std::uint32_t &id, const Tab &tab)
    {
        if (tabs.size() > id)
        {
            auto &realTab = tabs.at(id);

            std::unique_lock lock(Globals::gSoundsMutex);
            for (const auto &sound : realTab.sounds)
            {
                Globals::gSounds.erase(sound.id);
            }

            realTab = tab;

            for (auto &sound : realTab.sounds)
            {
                Globals::gSounds.insert({sound.id, sound});
            }
            return realTab;
        }

        Fancy::fancy.logTime().warning() << "Tried to access non existant Tab " << id << std::endl;
        return std::nullopt;
    }
    Data &Data::operator=(const Data &other)
    {
        if (this == &other)
        {
            return *this;
        }

        tabs = other.tabs;
        width = other.width;
        height = other.height;
        soundIdCounter = other.soundIdCounter;

        std::unique_lock lock(Globals::gSoundsMutex);
        Globals::gSounds.clear();

        for (std::size_t i = 0; tabs.size() > i; i++)
        {
            auto &tab = tabs.at(i);
            tab.id = i;

            for (auto &sound : tab.sounds)
            {
                Globals::gSounds.insert({sound.id, sound});
            }
        }

        return *this;
    }
} // namespace Soundux::Objects