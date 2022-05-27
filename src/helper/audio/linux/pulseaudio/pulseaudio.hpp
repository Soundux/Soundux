#if defined(__linux__)
#include "../backend.hpp"
#include "forward.hpp"
#include <map>
#include <mutex>
#include <optional>

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
            friend class AudioBackend;

          private:
            pa_context *context;
            pa_mainloop *mainloop;
            pa_mainloop_api *mainloopApi;

            //* ~= The modules we create =~
            std::optional<std::uint32_t> nullSink;
            std::optional<std::uint32_t> loopBack;
            std::optional<std::uint32_t> loopBackSink;

            std::optional<std::uint32_t> passthrough;
            std::optional<std::uint32_t> passthroughSink;
            std::optional<std::uint32_t> passthroughLoopBack;
            //* ~= ~~~~~~~~~~~~~~~~~~~~~ =~

            std::string serverName;

            std::string defaultSource;
            std::optional<std::uint32_t> defaultSourceId;

            std::map<std::string, std::uint32_t> movedApplications;
            std::map<std::string, std::uint32_t> movedPassthroughApplications;

            std::mutex operationMutex;

            void unloadLeftOvers();
            void fetchDefaultSource();
            void fetchLoopBackSinkId();
            void await(pa_operation *);

            void fixPlaybackApps(const std::vector<std::shared_ptr<PlaybackApp>> &);
            void fixRecordingApps(const std::vector<std::shared_ptr<RecordingApp>> &);

          protected:
            bool setup() override;

          public:
            //! Is not ran by default to avoid problems with switch-on-connect
            bool loadModules();

            void destroy() override;
            bool isRunningPipeWire();

            bool useAsDefault() override;
            bool revertDefault() override;
            bool muteInput(bool state) override;

            std::set<std::string> currentlyInputApps() override;
            std::set<std::string> currentlyPassedThrough() override;

            bool stopAllPassthrough() override;
            bool stopPassthrough(const std::string &app) override;
            bool passthroughFrom(std::shared_ptr<PlaybackApp> app) override;

            bool stopSoundInput() override;
            bool inputSoundTo(std::shared_ptr<RecordingApp> app) override;

            void unloadSwitchOnConnect();
            bool switchOnConnectPresent();

            std::shared_ptr<PlaybackApp> getPlaybackApp(const std::string &application) override;
            std::shared_ptr<RecordingApp> getRecordingApp(const std::string &application) override;

            std::vector<std::shared_ptr<PlaybackApp>> getPlaybackApps() override;
            std::vector<std::shared_ptr<RecordingApp>> getRecordingApps() override;
        };
    } // namespace Objects
} // namespace Soundux
#endif