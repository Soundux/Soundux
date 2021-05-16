#if defined(__linux__)
#include "../backend.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <pipewire/pipewire.h>

namespace Soundux
{
    namespace Objects
    {
        struct Port
        {
            char side;
            std::uint32_t id;
            std::string portAlias;
            spa_direction direction;
            std::uint32_t parentNode = 0;
        };

        struct Node
        {
            std::uint32_t id;
            std::string name;
            std::uint32_t pid;
            std::string applicationBinary;
            std::map<std::uint32_t, Port> ports;
        };

        struct PipeWirePlaybackApp : public PlaybackApp
        {
            std::uint32_t pid;
            std::uint32_t nodeId;
            ~PipeWirePlaybackApp() override = default;
        };
        struct PipeWireRecordingApp : public RecordingApp
        {
            std::uint32_t pid;
            std::uint32_t nodeId;
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

          private:
            std::mutex nodeLock;
            std::map<std::uint32_t, Node> nodes;

            std::mutex portLock;
            std::map<std::uint32_t, Port> ports;

            void onNodeInfo(const pw_node_info *);
            void onPortInfo(const pw_port_info *);

          private:
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