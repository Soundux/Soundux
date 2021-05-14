#if defined(__linux__)
#include "../backend.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <pipewire/core.h>
#include <pipewire/main-loop.h>
#include <pipewire/pipewire.h>

namespace Soundux
{
    namespace Objects
    {
        enum class Direction : std::uint8_t
        {
            FrontLeft,
            FrontRight
        };
        struct Link
        {
            std::uint32_t destination;
        };
        struct PipeWirePlaybackApp : public PlaybackApp
        {
            std::uint32_t id;
            Direction direction;
            std::vector<Link> links;
            ~PipeWirePlaybackApp() override = default;
        };

        struct PipeWireRecordingApp : public RecordingApp
        {
            std::uint32_t id;
            Direction direction;
            ~PipeWireRecordingApp() override = default;
        };

        class PipeWire : public AudioBackend
        {
            pw_core *core;
            pw_main_loop *loop;
            pw_context *context;

            pw_registry *registry;
            spa_hook registryListener;
            pw_registry_events registryEvents;

            std::mutex playbackMutex;
            std::vector<std::shared_ptr<PlaybackApp>> playbackApps;

            std::mutex recordingMutex;
            std::vector<std::shared_ptr<RecordingApp>> recordingApps;

          private:
            std::uint32_t nullSinkLeft;
            std::uint32_t nullSinkRight;

            std::uint32_t nullSinkPlaybackLeft;
            std::uint32_t nullSinkPlaybackRight;

            std::vector<std::uint32_t> soundInputLinks;
            std::vector<std::uint32_t> passthroughLinks;

          private:
            void sync();
            bool createNullSink();
            bool deleteLink(std::uint32_t);
            std::optional<int> linkPorts(std::uint32_t, std::uint32_t);

            static void onGlobalRemoved(void *, std::uint32_t);
            static void onGlobalAdded(void *, std::uint32_t, std::uint32_t, const char *, std::uint32_t,
                                      const spa_dict *);

          public:
            PipeWire() = default;
            void setup() override;
            void destroy() override;

            bool useAsDefault() override;
            bool revertDefault() override;
            bool muteInput(bool state) override;

            bool stopPassthrough() override;
            bool isCurrentlyPassingThrough() override;
            bool passthroughFrom(std::shared_ptr<PlaybackApp> app) override;

            bool stopSoundInput() override;
            bool inputSoundTo(std::shared_ptr<RecordingApp> app) override;

            std::shared_ptr<PlaybackApp> getPlaybackApp(const std::string &name) override;
            std::shared_ptr<RecordingApp> getRecordingApp(const std::string &name) override;

            std::vector<std::shared_ptr<PlaybackApp>> getPlaybackApps() override;
            std::vector<std::shared_ptr<RecordingApp>> getRecordingApps() override;
        };
    } // namespace Objects
} // namespace Soundux
#endif