#include "../backend.hpp"
#include <optional>
#include <regex>

#include <pulse/pulseaudio.h>

namespace Soundux
{
    // TODO(curve): Move this to pulse_forwarding.hpp
    //  namespace PulseAudioAPI
    //  {
    //      struct pa_context;
    //      struct pa_mainloop;
    //      struct pa_operation;
    //      struct pa_mainloop_api;
    //      struct pa_sink_input_info;
    //      struct pa_source_output_info;

    //     inline pa_mainloop *(*pa_mainloop_new)();
    //     inline int (*pa_mainloop_iterate)(pa_mainloop *, int, int *);
    //     inline pa_mainloop_api *(*pa_mainloop_get_api)(pa_mainloop *);
    //     inline pa_context *(*pa_context_new)(pa_mainloop_api *, const char *);
    //     inline int (*pa_context_connect)(pa_context *, const char *, unsigned int, const void *);

    //     using context_notify_cb = void (*)(pa_context *, void *);
    //     inline void (*pa_context_set_state_callback)(pa_context *, context_notify_cb, void *);

    //     using index_cb = void (*)(pa_context *, std::uint32_t, void *);
    //     inline pa_operation *(*pa_context_load_module)(pa_context *, const char *, const char *, index_cb, void *);

    //     using output_info_cb = void (*)(pa_context *, const pa_source_output_info *, int, void *);
    //     inline pa_operation *(*pa_context_get_source_output_info_list)(pa_context *, output_info_cb, void *);

    //     using input_info_cb = void (*)(pa_context *, const pa_sink_input_info *, int, void *);
    //     inline pa_operation *(*pa_context_get_sink_input_info_list)(pa_context *, input_info_cb, void *);

    //     enum class pa_context_state : std::uint8_t
    //     {
    //         Unconnected,
    //         Connecting,
    //         Authorizing,
    //         SettingName,
    //         Ready,
    //         Failed,
    //         Terminated
    //     };
    //     inline pa_context_state (*pa_context_get_state)(const pa_context *);
    // } // namespace PulseAudioAPI

    namespace Objects
    {
        struct PulsePlaybackApp : public PlaybackApp
        {
            std::uint32_t id;
            std::uint32_t pid;
            std::uint32_t sink;

            ~PulsePlaybackApp() override = default;
        };

        struct PulseRecordingApp : public RecordingApp
        {
            std::uint32_t id;
            std::uint32_t pid;
            std::uint32_t source;

            ~PulseRecordingApp() override = default;
        };

        class PulseAudio : public AudioBackend
        {
            pa_context *context;
            pa_mainloop *mainloop;
            pa_mainloop_api *mainloopApi;

            //* ~= The modules we create =~
            std::uint32_t nullSink;
            std::uint32_t loopBack;
            std::uint32_t loopBackSink;

            std::uint32_t passthrough;
            std::uint32_t passthroughSink;
            std::uint32_t passthroughLoopBack;
            //* ~= ~~~~~~~~~~~~~~~~~~~~~ =~

            std::string defaultSource;
            std::shared_ptr<PulseRecordingApp> movedApplication;
            std::shared_ptr<PulsePlaybackApp> movedPassthroughApplication;

            void unloadLeftOvers();
            void fetchDefaultSource();
            void fetchLoopBackSinkId();
            void await(pa_operation *);

            void fixPlaybackApps(const std::vector<std::shared_ptr<PlaybackApp>> &);
            void fixRecordingApps(const std::vector<std::shared_ptr<RecordingApp>> &);

          public:
            PulseAudio() = default;

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