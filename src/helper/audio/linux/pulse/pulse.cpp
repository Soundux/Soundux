#include "pulse.hpp"
#include <exception>
#include <fancy.hpp>
#include <helper/misc/misc.hpp>
#include <memory>

namespace Soundux::Objects
{
    void PulseAudio::setup()
    {
        mainloop = PulseApi::pa_mainloop_new();
        mainloopApi = PulseApi::pa_mainloop_get_api(mainloop);
        context = PulseApi::pa_context_new(mainloopApi, "soundux");
        PulseApi::pa_context_connect(context, nullptr, 0, nullptr);

        bool ready = false;
        PulseApi::pa_context_set_state_callback(
            context,
            [](pa_context *context, void *userData) {
                auto state = PulseApi::pa_context_get_state(context);
                if (state == PA_CONTEXT_FAILED)
                {
                    Fancy::fancy.logTime().failure() << "Failed to connect to pulseaudio" << std::endl;
                    std::terminate();
                }
                else if (state == PA_CONTEXT_READY)
                {
                    Fancy::fancy.logTime().message() << "PulseAudio is ready!" << std::endl;
                    *reinterpret_cast<bool *>(userData) = true;
                }
            },
            &ready);

        while (!ready)
        {
            PulseApi::pa_mainloop_iterate(mainloop, true, nullptr);
        }

        unloadLeftOvers();
        fetchDefaultSource();

        auto playbackApps = getPlaybackApps();
        auto recordingApps = getRecordingApps();

        await(PulseApi::pa_context_load_module(
            context, "module-null-sink",
            "sink_name=soundux_sink rate=44100 sink_properties=device.description=soundux_sink",
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load null sink" << std::endl;
                    std::terminate();
                }
                else
                {
                    *reinterpret_cast<std::uint32_t *>(userData) = id;
                }
            },
            &nullSink));

        await(PulseApi::pa_context_load_module(
            context, "module-loopback",
            ("rate=44100 source=" + defaultSource + " sink=soundux_sink sink_dont_move=true source_dont_move=true")
                .c_str(),
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load loopback" << std::endl;
                    std::terminate();
                }
                else
                {
                    *reinterpret_cast<std::uint32_t *>(userData) = id;
                }
            },
            &loopBack));

        await(PulseApi::pa_context_load_module(
            context, "module-null-sink",
            "sink_name=soundux_sink_passthrough rate=44100 sink_properties=device.description=soundux_sink_passthrough",
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load passthrough null sink" << std::endl;
                    std::terminate();
                }
                else
                {
                    *reinterpret_cast<std::uint32_t *>(userData) = id;
                }
            },
            &passthrough));

        await(PulseApi::pa_context_load_module(
            context, "module-loopback",
            "source=soundux_sink_passthrough.monitor sink=soundux_sink source_dont_move=true",
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load passthrough sink" << std::endl;
                    std::terminate();
                }
                else
                {
                    *reinterpret_cast<std::uint32_t *>(userData) = id;
                }
            },
            &passthroughSink));

        await(PulseApi::pa_context_load_module(
            context, "module-loopback", "source=soundux_sink_passthrough.monitor source_dont_move=true",
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load passthrough loopback" << std::endl;
                    std::terminate();
                }
                else
                {
                    *reinterpret_cast<std::uint32_t *>(userData) = id;
                }
            },
            &passthroughLoopBack));

        fetchLoopBackSinkId();
        fixPlaybackApps(playbackApps);
        fixRecordingApps(recordingApps);
    }
    void PulseAudio::destroy()
    {
        revertDefault();
        stopSoundInput();
        stopPassthrough();

        //* We only have to unload these 3 because the other modules depend on these and will automatically be deleted
        await(PulseApi::pa_context_unload_module(context, nullSink, nullptr, nullptr));
        await(PulseApi::pa_context_unload_module(context, loopBack, nullptr, nullptr));
        await(PulseApi::pa_context_unload_module(context, passthrough, nullptr, nullptr));
    }
    void PulseAudio::await(pa_operation *operation)
    {
        while (PulseApi::pa_operation_get_state(operation) != PA_OPERATION_DONE)
        {
            PulseApi::pa_mainloop_iterate(mainloop, true, nullptr);
        }
    }
    void PulseAudio::fetchDefaultSource()
    {
        await(PulseApi::pa_context_get_server_info(
            context,
            []([[maybe_unused]] pa_context *context, const pa_server_info *info, void *userData) {
                if (info)
                {
                    reinterpret_cast<PulseAudio *>(userData)->defaultSource = info->default_source_name;
                }
            },
            this));
    }
    void PulseAudio::fetchLoopBackSinkId()
    {
        auto data = std::make_pair(&loopBackSink, loopBack);

        await(PulseApi::pa_context_get_sink_input_info_list(
            context,
            []([[maybe_unused]] pa_context *ctx, const pa_sink_input_info *info, [[maybe_unused]] int eol,
               void *userData) {
                auto pair = *reinterpret_cast<decltype(data) *>(userData);
                if (info && info->owner_module && info->owner_module == pair.second)
                {
                    *pair.first = info->index;
                }
            },
            &data));
    }
    void PulseAudio::unloadLeftOvers()
    {
        await(PulseApi::pa_context_get_module_info_list(
            context,
            []([[maybe_unused]] pa_context *ctx, const pa_module_info *info, [[maybe_unused]] int eol, void *userData) {
                if (info && info->argument)
                {
                    if (std::string(info->argument).find("soundux") != std::string::npos)
                    {
                        auto *thiz = reinterpret_cast<PulseAudio *>(userData);
                        PulseApi::pa_context_unload_module(thiz->context, info->index, nullptr, nullptr);
                        Fancy::fancy.logTime().success() << "Unloaded left over module " << info->index << std::endl;
                    }
                }
            },
            this));
    }
    std::vector<std::shared_ptr<PlaybackApp>> PulseAudio::getPlaybackApps()
    {
        std::vector<std::shared_ptr<PlaybackApp>> rtn;
        await(PulseApi::pa_context_get_sink_input_info_list(
            context,
            []([[maybe_unused]] pa_context *ctx, const pa_sink_input_info *info, [[maybe_unused]] int eol,
               void *userData) {
                if (info && info->driver && std::strcmp(info->driver, "protocol-native.c") == 0)
                {
                    PulsePlaybackApp app;

                    app.id = info->index;
                    app.sink = info->sink;
                    app.name = PulseApi::pa_proplist_gets(info->proplist, "application.name");
                    app.pid = std::stoi(PulseApi::pa_proplist_gets(info->proplist, "application.process.id"));
                    app.application = PulseApi::pa_proplist_gets(info->proplist, "application.process.binary");
                    reinterpret_cast<decltype(rtn) *>(userData)->emplace_back(std::make_shared<PulsePlaybackApp>(app));
                }
            },
            &rtn));

        return rtn;
    }
    std::vector<std::shared_ptr<RecordingApp>> PulseAudio::getRecordingApps()
    {
        std::vector<std::shared_ptr<RecordingApp>> rtn;
        await(PulseApi::pa_context_get_source_output_info_list(
            context,
            []([[maybe_unused]] pa_context *ctx, const pa_source_output_info *info, [[maybe_unused]] int eol,
               void *userData) {
                if (info && info->driver && std::strcmp(info->driver, "protocol-native.c") == 0)
                {
                    if (info->resample_method && std::strcmp(info->resample_method, "peaks") == 0)
                    {
                        return;
                    }

                    PulseRecordingApp app;

                    app.id = info->index;
                    app.source = info->source;
                    app.name = PulseApi::pa_proplist_gets(info->proplist, "application.name");
                    app.pid = std::stoi(PulseApi::pa_proplist_gets(info->proplist, "application.process.id"));
                    app.application = PulseApi::pa_proplist_gets(info->proplist, "application.process.binary");
                    reinterpret_cast<decltype(rtn) *>(userData)->emplace_back(std::make_shared<PulseRecordingApp>(app));
                }
            },
            &rtn));

        return rtn;
    }
    bool PulseAudio::useAsDefault()
    {
        await(PulseApi::pa_context_unload_module(context, loopBack, nullptr, nullptr));

        await(PulseApi::pa_context_load_module(
            context, "module-loopback", ("rate=44100 source=" + defaultSource + " sink=soundux_sink").c_str(),
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load loopback" << std::endl;
                    std::terminate();
                }
                else
                {
                    *reinterpret_cast<std::uint32_t *>(userData) = id;
                }
            },
            &loopBack));

        bool success = false;
        await(PulseApi::pa_context_set_default_source(
            context, "soundux_sink.monitor",
            []([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                *reinterpret_cast<bool *>(userData) = success;
            },
            &success));

        if (!success)
        {
            Fancy::fancy.logTime().failure() << "Failed to set default source to soundux" << std::endl;
            return false;
        }

        return true;
    }
    bool PulseAudio::revertDefault()
    {
        if (!defaultSource.empty())
        {
            await(PulseApi::pa_context_unload_module(context, loopBack, nullptr, nullptr));

            auto result = std::make_pair(&loopBack, false);

            await(PulseApi::pa_context_load_module(
                context, "module-loopback",
                ("rate=44100 source=" + defaultSource + " sink=soundux_sink sink_dont_move=true source_dont_move=true")
                    .c_str(),
                []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                    auto *pair = reinterpret_cast<decltype(result) *>(userData);
                    *(pair->first) = id;
                    pair->second = id > 0;
                },
                &result));

            if (!result.second)
            {
                Fancy::fancy.logTime().failure() << "Failed to load loopback" << std::endl;
                return false;
            }

            bool success = false;
            await(PulseApi::pa_context_set_default_source(
                context, defaultSource.c_str(),
                []([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                    *reinterpret_cast<bool *>(userData) = success;
                },
                &success));

            if (!success)
            {
                Fancy::fancy.logTime().failure() << "Failed to reset default device" << std::endl;
                return false;
            }
        }

        return true;
    }
    bool PulseAudio::passthroughFrom(std::shared_ptr<PlaybackApp> app)
    {
        if (movedPassthroughApplication && movedPassthroughApplication->name == app->name)
        {
            Fancy::fancy.logTime().message()
                << "Ignoring sound passthrough request because requested app is already moved" << std::endl;
            return true;
        }
        if (!stopPassthrough())
        {
            Fancy::fancy.logTime().warning() << "Failed to stop current passthrough" << std::endl;
        }
        if (!app)
        {
            Fancy::fancy.logTime().warning() << "Tried to passthrough to non existant app" << std::endl;
            return false;
        }

        for (const auto &playbackApp : getPlaybackApps())
        {
            auto pulsePlayback = std::dynamic_pointer_cast<PulsePlaybackApp>(playbackApp);

            if (playbackApp->name == app->name)
            {
                bool success = true;

                await(PulseApi::pa_context_move_sink_input_by_name(
                    context, pulsePlayback->id, "soundux_sink_passthrough",
                    []([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                        if (!success)
                        {
                            *reinterpret_cast<bool *>(userData) = false;
                        }
                    },
                    &success));

                if (!success)
                {
                    Fancy::fancy.logTime().warning()
                        << "Failed top move " << pulsePlayback->id << " to passthrough" << std::endl;
                    return false;
                }
            }
        }

        movedPassthroughApplication = std::dynamic_pointer_cast<PulsePlaybackApp>(app);
        return true;
    }

    bool PulseAudio::stopPassthrough()
    {
        if (movedPassthroughApplication)
        {
            bool success = false;
            for (const auto &app : getPlaybackApps())
            {
                auto pulseApp = std::dynamic_pointer_cast<PulsePlaybackApp>(app);

                if (app->name == movedPassthroughApplication->name)
                {
                    await(PulseApi::pa_context_move_sink_input_by_index(
                        context, pulseApp->id, movedPassthroughApplication->sink,
                        []([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                            if (success)
                            {
                                *reinterpret_cast<bool *>(userData) = true;
                            }
                        },
                        &success));
                }
                movedPassthroughApplication.reset();
                return success;
            }
        }

        return true;
    }
    bool PulseAudio::inputSoundTo(std::shared_ptr<RecordingApp> app)
    {
        if (!app)
        {
            Fancy::fancy.logTime().warning() << "Tried to input sound to non existant app" << std::endl;
            return false;
        }
        if (movedApplication && movedApplication->name == app->name)
        {
            Fancy::fancy.logTime().message()
                << "Ignoring sound throughput request because sound is already throughput to requested app"
                << std::endl;
            return true;
        }

        stopSoundInput();

        for (const auto &recordingApp : getRecordingApps())
        {
            auto pulseApp = std::dynamic_pointer_cast<PulseRecordingApp>(recordingApp);

            if (pulseApp->name == app->name)
            {
                bool success = true;
                await(PulseApi::pa_context_move_source_output_by_name(
                    context, pulseApp->id, "soundux_sink.monitor",
                    []([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                        if (!success)
                        {
                            *reinterpret_cast<bool *>(userData) = false;
                        }
                    },
                    &success));

                if (!success)
                {
                    Fancy::fancy.logTime().warning() << "Failed to move " + pulseApp->name << "(" << pulseApp->id
                                                     << ") to soundux sink" << std::endl;
                }
            }
        }

        movedApplication = std::dynamic_pointer_cast<PulseRecordingApp>(app);
        return true;
    }
    bool PulseAudio::stopSoundInput()
    {
        bool success = true;
        if (movedApplication)
        {
            for (const auto &recordingApp : getRecordingApps())
            {
                auto pulseApp = std::dynamic_pointer_cast<PulseRecordingApp>(recordingApp);

                if (pulseApp->name == movedApplication->name)
                {
                    await(PulseApi::pa_context_move_source_output_by_index(
                        context, pulseApp->id, movedApplication->source,
                        []([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                            if (!success)
                            {
                                *reinterpret_cast<bool *>(userData) = false;
                            }
                        },
                        &success));

                    if (!success)
                    {
                        Fancy::fancy.logTime().warning() << "Failed to move " << pulseApp->name << "(" << pulseApp->id
                                                         << ") back to original source" << std::endl;
                        success = false;
                    }
                }
            }
            movedApplication.reset();
        }

        return success;
    }
    std::shared_ptr<PlaybackApp> PulseAudio::getPlaybackApp(const std::string &name)
    {
        for (auto app : getPlaybackApps())
        {
            if (app->name == name)
            {
                return app;
            }
        }

        return nullptr;
    }
    std::shared_ptr<RecordingApp> PulseAudio::getRecordingApp(const std::string &name)
    {
        for (auto app : getRecordingApps())
        {
            if (app->name == name)
            {
                return app;
            }
        }

        return nullptr;
    }

    void PulseAudio::fixPlaybackApps(const std::vector<std::shared_ptr<PlaybackApp>> &originalPlayback)
    {
        for (const auto &playbackApp : getPlaybackApps())
        {
            auto pulsePlaybackApp = std::dynamic_pointer_cast<PulsePlaybackApp>(playbackApp);
            auto originalPlaybackApp =
                std::find_if(originalPlayback.begin(), originalPlayback.end(), [&pulsePlaybackApp](const auto &o) {
                    return pulsePlaybackApp->id == std::dynamic_pointer_cast<PulsePlaybackApp>(o)->id;
                });

            if (originalPlaybackApp != originalPlayback.end())
            {
                auto *pulseOriginal = dynamic_cast<PulsePlaybackApp *>(originalPlaybackApp->get());
                if (pulseOriginal->sink != pulsePlaybackApp->sink)
                {
                    await(PulseApi::pa_context_move_sink_input_by_index(context, pulsePlaybackApp->id,
                                                                        pulseOriginal->sink, nullptr, nullptr));
                    Fancy::fancy.logTime().success()
                        << "Recovered " << pulsePlaybackApp->id << " from soundux passthrough" << std::endl;
                }
            }
        }
    }
    void PulseAudio::fixRecordingApps(const std::vector<std::shared_ptr<RecordingApp>> &originalRecording)
    {
        for (const auto &recordingApp : getRecordingApps())
        {
            auto pulseRecordingApp = std::dynamic_pointer_cast<PulseRecordingApp>(recordingApp);
            auto originalRecordingApp =
                std::find_if(originalRecording.begin(), originalRecording.end(), [&pulseRecordingApp](const auto &o) {
                    return pulseRecordingApp->id == std::dynamic_pointer_cast<PulseRecordingApp>(o)->id;
                });

            if (originalRecordingApp != originalRecording.end())
            {
                auto *pulseOriginal = dynamic_cast<PulseRecordingApp *>(originalRecordingApp->get());
                if (pulseOriginal->source != pulseRecordingApp->source)
                {
                    await(PulseApi::pa_context_move_source_output_by_index(context, pulseRecordingApp->id,
                                                                           pulseOriginal->source, nullptr, nullptr));
                    Fancy::fancy.logTime().success()
                        << "Recovered " << pulseRecordingApp->id << " from soundux sink" << std::endl;
                }
            }
        }
    }
    bool PulseAudio::muteInput(bool state)
    {
        bool success = false;

        await(PulseApi::pa_context_set_sink_input_mute(
            context, loopBackSink, state,
            +[]([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                *reinterpret_cast<bool *>(userData) = success;
            },
            &success));

        if (!success)
        {
            Fancy::fancy.logTime().failure() << "Failed to mute loopback sink" << std::endl;
        }

        return success;
    }

    bool PulseAudio::isCurrentlyPassingThrough()
    {
        return movedPassthroughApplication != nullptr;
    }
} // namespace Soundux::Objects