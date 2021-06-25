#pragma once
#include <atomic>
#include <core/objects/objects.hpp>
#include <helper/audio/device/device.hpp>
#include <lock.hpp>
#include <memory>
#include <optional>

namespace Soundux
{
    namespace Objects
    {
        class PlayingSound : public std::enable_shared_from_this<PlayingSound>
        {
            sxl::lock<std::shared_ptr<ma_device>> device;
            sxl::lock<std::shared_ptr<ma_decoder>> decoder;

          private:
            AudioDevice playbackDevice;

            std::uint64_t length{0};
            std::uint64_t lengthInMs{0};
            std::uint64_t readFrames{0};
            std::uint32_t sampleRate{0};

            std::atomic<bool> done{false};
            std::atomic<bool> paused{false};
            std::atomic<bool> shouldRepeat{false};

            std::atomic<std::uint64_t> readInMs{0};
            std::atomic<std::optional<std::uint64_t>> seekTo;

            Sound sound;
            std::uint64_t buffer{0};

            std::uint64_t id;
            static std::atomic<std::uint64_t> idCounter;

          private:
            PlayingSound() = default;

            void onFinished();
            void onProgressed(const std::uint64_t &frames);
            static void data_callback(ma_device *, void *, const void *, std::uint32_t);

          public:
            void stop();
            void pause();
            void resume();

            //* In Milliseconds
            std::uint64_t getRead() const;
            std::uint64_t getLength() const;

            void repeat(bool);
            void setVolume(float);
            void seek(const std::uint64_t &);

            bool isPaused() const;
            Sound getSound() const;
            bool isRepeating() const;
            std::uint64_t getId() const;
            AudioDevice getPlaybackDevice() const;

          public:
            ~PlayingSound();
            static std::shared_ptr<PlayingSound> create(const Sound &, const AudioDevice &);
        };
    } // namespace Objects
} // namespace Soundux