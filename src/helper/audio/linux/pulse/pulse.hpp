#include "../backend.hpp"
#include "forward.hpp"
#include <optional>
#include <regex>

namespace Soundux
{
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