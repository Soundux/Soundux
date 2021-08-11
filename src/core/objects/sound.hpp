#pragma once
#include <atomic>
#include <core/hotkeys/keys.hpp>
#include <cstdint>
#include <lock.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Soundux
{
    namespace Objects
    {
        struct Sound
        {
            std::uint64_t id;
            std::string name;
            std::string path;

            sxl::lock<std::vector<Key>> hotkeys;
            sxl::lock<std::optional<std::uint32_t>> localVolume;
            sxl::lock<std::optional<std::uint32_t>> remoteVolume;

            std::atomic<bool> favorite = false;
            std::atomic<std::uint64_t> modifiedDate;

          public:
            Sound(std::uint64_t, std::string, std::string);

          public:
            bool isFavorite() const;
            void markFavorite(bool);

            std::vector<Key> getHotkeys() const;
            void setHotkeys(const std::vector<Key> &);

            void setModifiedDate(std::uint64_t);
            std::uint64_t getModifiedDate() const;

            std::string getName() const;
            std::string getPath() const;
            std::uint64_t getId() const;

            std::optional<int> getLocalVolume() const;
            std::optional<int> getRemoteVolume() const;
            void setLocalVolume(std::optional<std::uint32_t>);
            void setRemoteVolume(std::optional<std::uint32_t>);
        };
    } // namespace Objects
} // namespace Soundux