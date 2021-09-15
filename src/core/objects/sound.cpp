#include "sound.hpp"

namespace Soundux::Objects
{
    Sound::Sound(std::uint64_t id, std::string name, std::string path)
        : id(id), name(std::move(name)), path(std::move(path))
    {
    }

    bool Sound::isFavorite() const
    {
        return favorite;
    }
    void Sound::markFavorite(bool state)
    {
        // TODO(): Modify gFavorites
        favorite = state;
    }

    std::vector<Key> Sound::getHotkeys() const
    {
        return hotkeys.copy();
    }
    void Sound::setHotkeys(const std::vector<Key> &keys)
    {
        hotkeys.assign(keys);
    }

    void Sound::setModifiedDate(std::uint64_t newDate)
    {
        modifiedDate = newDate;
    }
    std::uint64_t Sound::getModifiedDate() const
    {
        return modifiedDate;
    }

    std::string Sound::getName() const
    {
        return name;
    }
    std::string Sound::getPath() const
    {
        return path;
    }
    std::uint64_t Sound::getId() const
    {
        return id;
    }

    std::optional<int> Sound::getLocalVolume() const
    {
        return localVolume.copy();
    }
    std::optional<int> Sound::getRemoteVolume() const
    {
        return localVolume.copy();
    }
    void Sound::setLocalVolume(std::optional<std::uint32_t> newVolume)
    {
        localVolume.assign(newVolume);
    }
    void Sound::setRemoteVolume(std::optional<std::uint32_t> newVolume)
    {
        remoteVolume.assign(newVolume);
    }
} // namespace Soundux::Objects