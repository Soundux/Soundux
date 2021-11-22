#if defined(__linux__)
#include "../backend.hpp"
#include <lock.hpp>
#include <map>
#include <optional>

#include <pipewire/extensions/metadata.h>
#include <pipewire/global.h>
#include <pipewire/pipewire.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>

namespace Soundux
{
    namespace Objects
    {
        struct Node
        {
            std::uint32_t id;
            std::string name;
            std::uint32_t pid;
            std::string rawName;
            bool isMonitor = false;
            std::uint32_t inputPorts;
            std::uint32_t outputPorts;
            std::string applicationBinary;
        };

        struct PipeWirePlaybackApp : public PlaybackApp
        {
            Node node;
            ~PipeWirePlaybackApp() override = default;
        };
        struct PipeWireRecordingApp : public RecordingApp
        {
            Node node;
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
            std::string defaultMicrophone;

            spa_hook registryListener;
            pw_registry_events registryEvents;

          private:
            sxl::lock<std::map<std::uint32_t, Node>> nodes;

            void onNodeInfo(const pw_node_info *);
            void onCoreInfo(const pw_core_info *);

          private:
            std::map<std::string, std::vector<std::uint32_t>> soundInputLinks;
            std::map<std::string, std::vector<std::uint32_t>> passthroughLinks;

          private:
            void sync();
            bool createNullSink();
            bool deleteLink(std::uint32_t);
            std::optional<int> createLink(std::uint32_t, std::uint32_t);

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

            std::set<std::string> currentlyInputApps() override;
            std::set<std::string> currentlyPassedThrough() override;

            bool stopAllPassthrough() override;
            bool stopPassthrough(const std::string &app) override;
            bool passthroughFrom(const std::string &app) override;

            bool stopSoundInput() override;
            bool inputSoundTo(const std::string &app) override;

            std::vector<std::shared_ptr<PlaybackApp>> getPlaybackApps() override;
            std::vector<std::shared_ptr<RecordingApp>> getRecordingApps() override;
        };
    } // namespace Objects
} // namespace Soundux
#endif