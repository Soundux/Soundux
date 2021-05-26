#if defined(__linux__)
#include "../backend.hpp"
#include <map>
#include <optional>
#include <pipewire/pipewire.h>
#include <var_guard.hpp>

// TODO(pipewire):
//* From the pipewire news of 0.3.26
//*   - The link factory can now also make links between nodes and
//*     ports by name so that it can be used in scripts.
//*
//* Maybe we could try to make use of that, it could reduce loc.

namespace Soundux
{
    namespace Objects
    {
        enum class Side
        {
            UNDEFINED,
            LEFT,
            RIGHT,
            MONO,
        };

        struct Port
        {
            std::uint32_t id;
            std::string portAlias;
            spa_direction direction;
            Side side = Side::UNDEFINED;
            std::uint32_t parentNode = 0;
        };

        struct Node
        {
            std::uint32_t id;
            std::string name;
            std::uint32_t pid;
            bool isMonitor = false;
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
            friend class AudioBackend;

          private:
            pw_core *core;
            pw_main_loop *loop;
            pw_context *context;
            pw_registry *registry;
            std::uint32_t version = 0;

            spa_hook registryListener;
            pw_registry_events registryEvents;

          private:
            sxl::var_guard<std::map<std::uint32_t, Node>> nodes;
            sxl::var_guard<std::map<std::uint32_t, Port>> ports;

            void onNodeInfo(const pw_node_info *);
            void onPortInfo(const pw_port_info *);
            void onCoreInfo(const pw_core_info *);

          private:
            std::map<std::string, std::vector<std::uint32_t>> soundInputLinks;
            std::map<std::string, std::vector<std::uint32_t>> passthroughLinks;

          private:
            void sync();
            bool createNullSink();
            bool deleteLink(std::uint32_t);
            std::optional<int> linkPorts(std::uint32_t, std::uint32_t);

            static void onGlobalRemoved(void *, std::uint32_t);
            static void onGlobalAdded(void *, std::uint32_t, std::uint32_t, const char *, std::uint32_t,
                                      const spa_dict *);

          protected:
            bool setup() override;

          public:
            PipeWire() = default;
            void destroy() override;

            bool useAsDefault() override;
            bool revertDefault() override;
            bool muteInput(bool state) override;

            bool stopAllPassthrough() override;
            bool isCurrentlyPassingThrough() override;
            std::size_t passedThroughApplications() override;
            bool stopPassthrough(const std::string &name) override;
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