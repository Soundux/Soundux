#include "data.hpp"
#include <core/global/globals.hpp>
#include <fancy.hpp>

namespace Soundux::Objects
{
    Tab Data::addTab(Tab tab)
    {
        tab.id = tabs.size();
        tabs.emplace_back(tab);

        for (auto &sound : tabs.back().sounds)
        {
            Globals::gSounds->insert({sound.id, sound});
            if (sound.isFavorite)
            {
                Globals::gFavorites->insert({sound.id, sound});
            }
        }

        return tabs.back();
    }
    void Data::removeTabById(const std::uint32_t &index)
    {
        if (tabs.size() > index)
        {
            auto &tab = tabs.at(index);
            for (auto &sound : tab.sounds)
            {
                Globals::gSounds->erase(sound.id);
                if (sound.isFavorite)
                {
                    Globals::gFavorites->erase(sound.id);
                }
            }

            tabs.erase(tabs.begin() + index);

            for (std::size_t i = 0; tabs.size() > i; i++)
            {
                tabs.at(i).id = i;
            }
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Tried to remove non existent tab" << std::endl;
        }
    }
    void Data::setTabs(const std::vector<Tab> &newTabs)
    {
        tabs = newTabs;
        Globals::gSounds->clear();
        Globals::gFavorites->clear();
        for (std::size_t i = 0; tabs.size() > i; i++)
        {
            auto &tab = tabs.at(i);
            tab.id = i;

            for (auto &sound : tab.sounds)
            {
                Globals::gSounds->insert({sound.id, sound});
                if (sound.isFavorite)
                {
                    Globals::gFavorites->insert({sound.id, sound});
                }
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

        Fancy::fancy.logTime().warning() << "Tried to access non existent tab " << id << std::endl;
        return std::nullopt;
    }
    std::optional<std::reference_wrapper<Sound>> Data::getSound(const std::uint32_t &id)
    {
        auto scopedSounds = Globals::gSounds.scoped();
        if (scopedSounds->find(id) != scopedSounds->end())
        {
            return scopedSounds->at(id);
        }

        Fancy::fancy.logTime().warning() << "Tried to access non existent sound " << id << std::endl;
        return std::nullopt;
    }
    std::optional<Tab> Data::setTab(const std::uint32_t &id, const Tab &tab)
    {
        if (tabs.size() > id)
        {
            auto &realTab = tabs.at(id);

            for (const auto &sound : realTab.sounds)
            {
                Globals::gSounds->erase(sound.id);
                if (sound.isFavorite)
                {
                    Globals::gFavorites->erase(sound.id);
                }
            }

            realTab = tab;

            for (auto &sound : realTab.sounds)
            {
                Globals::gSounds->insert({sound.id, sound});
                if (sound.isFavorite)
                {
                    Globals::gFavorites->insert({sound.id, sound});
                }
            }
            return realTab;
        }

        Fancy::fancy.logTime().warning() << "Tried to access non existent Tab " << id << std::endl;
        return std::nullopt;
    }
    void Data::set(const Data &other)
    {
        tabs = other.tabs;
        width = other.width;
        height = other.height;
        soundIdCounter = other.soundIdCounter;

        Globals::gSounds->clear();
        Globals::gFavorites->clear();

        for (std::size_t i = 0; tabs.size() > i; i++)
        {
            auto &tab = tabs.at(i);
            tab.id = i;

            for (auto &sound : tab.sounds)
            {
                Globals::gSounds->insert({sound.id, sound});
                if (sound.isFavorite)
                {
                    Globals::gFavorites->insert({sound.id, sound});
                }
            }
        }
    }
    void Data::markFavorite(const std::uint32_t &id, bool favourite)
    {
        auto sound = getSound(id);
        if (sound)
        {
            sound->get().isFavorite = favourite;
            if (favourite)
            {
                Globals::gFavorites->insert({id, sound->get()});
            }
            else
            {
                auto scopedFavorites = Globals::gFavorites.scoped();
                if (scopedFavorites->find(id) != scopedFavorites->end())
                {
                    scopedFavorites->erase(id);
                }
            }
        }
    }
    std::vector<std::uint32_t> Data::getFavoriteIds()
    {
        auto scopedFavorites = Globals::gFavorites.scoped();

        std::vector<std::uint32_t> rtn;
        rtn.reserve(scopedFavorites->size());

        for (const auto &sound : *scopedFavorites)
        {
            rtn.emplace_back(sound.first);
        }

        return rtn;
    }
    std::vector<Sound> Data::getFavorites()
    {
        auto scopedFavorites = Globals::gFavorites.scoped();

        std::vector<Sound> rtn;
        rtn.reserve(scopedFavorites->size());

        for (const auto &sound : *scopedFavorites)
        {
            rtn.emplace_back(sound.second);
        }

        return rtn;
    }
    bool Data::doesTabExist(const std::string &path)
    {
        auto it = std::find_if(tabs.begin(), tabs.end(), [&](const auto &tab) { return tab.path == path; });
        return it != tabs.end();
    }
} // namespace Soundux::Objects