#if defined(__linux__)
#include "pulseaudio.hpp"
#include "forward.hpp"
#include <core/global/globals.hpp>
#include <cstring>
#include <exception>
#include <fancy.hpp>

namespace Soundux::Objects
{
    bool PulseAudio::setup()
    {
        if (!PulseApi::setup())
        {
            return false;
        }

        mainloop = PulseApi::mainloop_new();
        mainloopApi = PulseApi::mainloop_get_api(mainloop);
        context = PulseApi::context_new(mainloopApi, "soundux");
        PulseApi::context_connect(context, nullptr, pa_context_flags::PA_CONTEXT_NOFLAGS, nullptr);

        auto data = std::make_pair(false, false);
        PulseApi::context_set_state_callback(
            context,
            [](pa_context *context, void *userData) {
                auto state = PulseApi::context_get_state(context);
                auto *data = reinterpret_cast<std::pair<bool, bool> *>(userData);

                if (!data)
                {
                    return;
                }

                if (state == PA_CONTEXT_FAILED)
                {
                    Fancy::fancy.logTime().failure() << "Failed to connect to pulseaudio" << std::endl;
                    data->first = true;
                    data->second = false;
                }
                else if (state == PA_CONTEXT_READY)
                {
                    Fancy::fancy.logTime().message() << "PulseAudio is ready!" << std::endl;
                    data->first = true;
                    data->second = true;
                }
            },
            &data);

        while (!data.first)
        {
            PulseApi::mainloop_iterate(mainloop, true, nullptr);
        }

        if (!data.second)
        {
            return false;
        }

        unloadLeftOvers();
        unloadProblematic();
        fetchDefaultSource();

        if (defaultSource.empty() || serverName.empty() || isRunningPipeWire())
        {
            return false;
        }

        return loadModules();
    }
    void PulseAudio::unloadProblematic()
    {
        await(PulseApi::context_get_module_info_list(
            context,
            []([[maybe_unused]] pa_context *ctx, const pa_module_info *info, [[maybe_unused]] int eol, void *userData) {
                if (info && info->name && userData)
                {
                    auto name = std::string(info->name);
                    auto *thiz = reinterpret_cast<PulseAudio *>(userData);

                    if (name.find("switch-on-connect") != std::string::npos ||
                        name.find("role-cork") != std::string::npos)
                    {
                        PulseModule module;
                        module.name = name;

                        if (info->argument)
                            module.arguments = info->argument;

                        thiz->unloadedModules.emplace_back(module);
                        Fancy::fancy.logTime().message() << "Unloading problematic module: " << name << std::endl;
                    }
                }
            },
            this));
    }
    void PulseAudio::reloadProblematic()
    {
        for (auto &module : unloadedModules)
        {
            Fancy::fancy.logTime().message() << "Loading back module: " << module.name << std::endl;
            PulseApi::context_load_module(context, module.name.c_str(), module.arguments.c_str(), nullptr, nullptr);
        }
    }
    bool PulseAudio::loadModules()
    {
        auto playbackApps = getPlaybackApps();
        auto recordingApps = getRecordingApps();

        await(PulseApi::context_load_module(
            context, "module-null-sink",
            "sink_name=soundux_sink rate=44100 sink_properties=device.description=soundux_sink",
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load null sink" << std::endl;
                }
                else
                {
                    *reinterpret_cast<std::optional<std::uint32_t> *>(userData) = id;
                }
            },
            &nullSink));

        await(PulseApi::context_load_module(
            context, "module-loopback",
            ("rate=44100 source=" + defaultSource + " sink=soundux_sink sink_dont_move=true source_dont_move=true")
                .c_str(),
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load loopback" << std::endl;
                }
                else
                {
                    *reinterpret_cast<std::optional<std::uint32_t> *>(userData) = id;
                }
            },
            &loopBack));

        await(PulseApi::context_load_module(
            context, "module-null-sink",
            "sink_name=soundux_sink_passthrough rate=44100 sink_properties=device.description=soundux_sink_passthrough",
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load passthrough null sink" << std::endl;
                }
                else
                {
                    *reinterpret_cast<std::optional<std::uint32_t> *>(userData) = id;
                }
            },
            &passthrough));

        await(PulseApi::context_load_module(
            context, "module-loopback",
            "source=soundux_sink_passthrough.monitor sink=soundux_sink source_dont_move=true",
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load passthrough sink" << std::endl;
                }
                else
                {
                    *reinterpret_cast<std::optional<std::uint32_t> *>(userData) = id;
                }
            },
            &passthroughSink));

        await(PulseApi::context_load_module(
            context, "module-loopback", "source=soundux_sink_passthrough.monitor source_dont_move=true",
            []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                if (static_cast<int>(id) < 0)
                {
                    Fancy::fancy.logTime().failure() << "Failed to load passthrough loopback" << std::endl;
                }
                else
                {
                    *reinterpret_cast<std::optional<std::uint32_t> *>(userData) = id;
                }
            },
            &passthroughLoopBack));

        fetchLoopBackSinkId();

        if (!nullSink || !loopBack || !loopBackSink || !passthrough || !passthroughSink || !passthroughLoopBack ||
            !defaultSourceId)
        {
            unloadLeftOvers();
            return false;
        }

        fixPlaybackApps(playbackApps);
        fixRecordingApps(recordingApps);

        return true;
    }
    void PulseAudio::destroy()
    {
        revertDefault();
        stopSoundInput();
        stopAllPassthrough();

        if (nullSink)
            await(PulseApi::context_unload_module(context, *nullSink, nullptr, nullptr));
        if (loopBack)
            await(PulseApi::context_unload_module(context, *loopBack, nullptr, nullptr));
        if (loopBackSink)
            await(PulseApi::context_unload_module(context, *loopBackSink, nullptr, nullptr));

        if (passthrough)
            await(PulseApi::context_unload_module(context, *passthrough, nullptr, nullptr));
        if (passthroughSink)
            await(PulseApi::context_unload_module(context, *passthroughSink, nullptr, nullptr));
        if (passthroughLoopBack)
            await(PulseApi::context_unload_module(context, *passthroughLoopBack, nullptr, nullptr));

        reloadProblematic();
    }
    void PulseAudio::await(pa_operation *operation)
    {
        std::lock_guard lock(operationMutex);
        while (PulseApi::operation_get_state(operation) != PA_OPERATION_DONE)
        {
            PulseApi::mainloop_iterate(mainloop, true, nullptr);
        }
    }
    void PulseAudio::fetchDefaultSource()
    {
        await(PulseApi::context_get_server_info(
            context,
            []([[maybe_unused]] pa_context *context, const pa_server_info *info, void *userData) {
                if (info)
                {
                    reinterpret_cast<PulseAudio *>(userData)->defaultSource = info->default_source_name;
                    reinterpret_cast<PulseAudio *>(userData)->serverName = info->server_name;
                }
            },
            this));
        await(PulseApi::context_get_sink_input_info_list(
            context,
            []([[maybe_unused]] pa_context *ctx, const pa_sink_input_info *info, [[maybe_unused]] int eol,
               void *userData) {
                auto *thiz = reinterpret_cast<PulseAudio *>(userData);
                if (info && info->name == thiz->defaultSource)
                {
                    thiz->defaultSourceId = info->index;
                }
            },
            this));
    }
    void PulseAudio::fetchLoopBackSinkId()
    {
        auto data = std::make_pair(&loopBackSink, loopBack);

        await(PulseApi::context_get_sink_input_info_list(
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
        await(PulseApi::context_get_module_info_list(
            context,
            []([[maybe_unused]] pa_context *ctx, const pa_module_info *info, [[maybe_unused]] int eol, void *userData) {
                if (info && info->argument)
                {
                    if (std::string(info->argument).find("soundux") != std::string::npos)
                    {
                        auto *thiz = reinterpret_cast<PulseAudio *>(userData);
                        PulseApi::context_unload_module(thiz->context, info->index, nullptr, nullptr);
                        Fancy::fancy.logTime().success() << "Unloaded left over module " << info->index << std::endl;
                    }
                }
            },
            this));
    }
    std::vector<std::shared_ptr<PlaybackApp>> PulseAudio::getPlaybackApps()
    {
        std::vector<std::shared_ptr<PlaybackApp>> rtn;
        await(PulseApi::context_get_sink_input_info_list(
            context,
            []([[maybe_unused]] pa_context *ctx, const pa_sink_input_info *info, [[maybe_unused]] int eol,
               void *userData) {
                if (info && info->driver && std::strcmp(info->driver, "protocol-native.c") == 0)
                {
                    PulsePlaybackApp app;

                    app.id = info->index;
                    app.sink = info->sink;
                    app.name = PulseApi::proplist_gets(info->proplist, "application.name");
                    app.pid = std::stoi(PulseApi::proplist_gets(info->proplist, "application.process.id"));
                    app.application = PulseApi::proplist_gets(info->proplist, "application.process.binary");
                    reinterpret_cast<decltype(rtn) *>(userData)->emplace_back(std::make_shared<PulsePlaybackApp>(app));
                }
            },
            &rtn));

        return rtn;
    }
    std::vector<std::shared_ptr<RecordingApp>> PulseAudio::getRecordingApps()
    {
        std::vector<std::shared_ptr<RecordingApp>> rtn;
        await(PulseApi::context_get_source_output_info_list(
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
                    app.name = PulseApi::proplist_gets(info->proplist, "application.name");
                    app.pid = std::stoi(PulseApi::proplist_gets(info->proplist, "application.process.id"));
                    app.application = PulseApi::proplist_gets(info->proplist, "application.process.binary");
                    reinterpret_cast<decltype(rtn) *>(userData)->emplace_back(std::make_shared<PulseRecordingApp>(app));
                }
            },
            &rtn));

        return rtn;
    }
    bool PulseAudio::useAsDefault()
    {
        if (!defaultSource.empty())
        {
            await(PulseApi::context_unload_module(context, *loopBack, nullptr, nullptr));

            await(PulseApi::context_load_module(
                context, "module-loopback", ("rate=44100 source=" + defaultSource + " sink=soundux_sink").c_str(),
                []([[maybe_unused]] pa_context *m, std::uint32_t id, void *userData) {
                    if (static_cast<int>(id) < 0)
                    {
                        Fancy::fancy.logTime().failure() << "Failed to load loopback" << std::endl;
                        *reinterpret_cast<std::optional<std::uint32_t> *>(userData) = std::nullopt;
                    }
                    else
                    {
                        *reinterpret_cast<std::optional<std::uint32_t> *>(userData) = id;
                    }
                },
                &loopBack));

            if (!loopBack)
            {
                return false;
            }

            bool success = false;
            await(PulseApi::context_set_default_source(
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

        Fancy::fancy.logTime().failure() << "Could not set default source because original default source is unknown"
                                         << std::endl;
        return false;
    }
    bool PulseAudio::revertDefault()
    {
        if (!defaultSource.empty() && loopBack)
        {
            await(PulseApi::context_unload_module(context, *loopBack, nullptr, nullptr));

            auto result = std::make_pair(&loopBack, false);

            await(PulseApi::context_load_module(
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
            await(PulseApi::context_set_default_source(
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
    bool PulseAudio::passthroughFrom(const std::string &app)
    {
        if (movedPassthroughApplications.count(app))
        {
            Fancy::fancy.logTime().message()
                << "Ignoring sound passthrough request because requested app is already moved" << std::endl;
            return true;
        }

        if (app.empty())
        {
            Fancy::fancy.logTime().warning() << "Tried to passthrough to non existant app" << std::endl;
            return false;
        }

        std::uint32_t originalSink = 0;
        for (const auto &playbackApp : getPlaybackApps())
        {
            auto pulsePlayback = std::dynamic_pointer_cast<PulsePlaybackApp>(playbackApp);

            if (playbackApp->application == app)
            {
                bool success = true;

                await(PulseApi::context_move_sink_input_by_name(
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

                originalSink = pulsePlayback->sink;
            }
        }

        movedPassthroughApplications.emplace(app, originalSink);
        return true;
    }
    bool PulseAudio::stopAllPassthrough()
    {
        bool success = true;
        for (const auto &[movedAppBinary, originalSource] : movedPassthroughApplications)
        {
            for (const auto &app : getPlaybackApps())
            {
                auto pulseApp = std::dynamic_pointer_cast<PulsePlaybackApp>(app);

                if (app->application == movedAppBinary)
                {
                    await(PulseApi::context_move_sink_input_by_index(
                        context, pulseApp->id, originalSource,
                        []([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                            if (!success)
                            {
                                *reinterpret_cast<bool *>(userData) = false;
                            }
                        },
                        &success));
                }
            }
        }
        movedPassthroughApplications.clear();

        if (!success)
        {
            Fancy::fancy.logTime().warning() << "Failed to move back one or more applications" << std::endl;
        }

        return success;
    }
    bool PulseAudio::stopPassthrough(const std::string &app)
    {
        if (movedPassthroughApplications.find(app) != movedPassthroughApplications.end())
        {
            bool success = true;
            auto &originalSource = movedPassthroughApplications.at(app);
            for (const auto &playbackApp : getPlaybackApps())
            {
                auto pulseApp = std::dynamic_pointer_cast<PulsePlaybackApp>(playbackApp);

                if (playbackApp->application == app)
                {
                    await(PulseApi::context_move_sink_input_by_index(
                        context, pulseApp->id, originalSource,
                        []([[maybe_unused]] pa_context *ctx, int success, void *userData) {
                            if (!success)
                            {
                                *reinterpret_cast<bool *>(userData) = false;
                            }
                        },
                        &success));
                }
            }

            movedPassthroughApplications.erase(app);
            if (!success)
            {
                Fancy::fancy.logTime().warning() << "Failed to move back passthrough for " << app << std::endl;
            }
        }

        Fancy::fancy.logTime().warning() << "Could not find moved application " << app << std::endl;
        return false;
    }
    bool PulseAudio::inputSoundTo(const std::string &app)
    {
        if (app.empty())
        {
            Fancy::fancy.logTime().warning() << "Tried to input sound to non existant app" << std::endl;
            return false;
        }

        if (movedApplications.find(app) != movedApplications.end())
        {
            return true;
        }

        std::uint32_t originalSource = 0;
        for (const auto &recordingApp : getRecordingApps())
        {
            auto pulseApp = std::dynamic_pointer_cast<PulseRecordingApp>(recordingApp);

            if (pulseApp->application == app)
            {
                bool success = true;
                await(PulseApi::context_move_source_output_by_name(
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
                    Fancy::fancy.logTime().warning() << "Failed to move " + pulseApp->application << "(" << pulseApp->id
                                                     << ") to soundux sink" << std::endl;
                }
                else
                {
                    originalSource = pulseApp->source;
                }
            }
        }

        movedApplications.emplace(app, originalSource);
        return true;
    }
    bool PulseAudio::stopSoundInput()
    {
        bool success = true;

        for (const auto &[movedAppBinary, originalSink] : movedApplications)
        {
            for (const auto &recordingApp : getRecordingApps())
            {
                auto pulseApp = std::dynamic_pointer_cast<PulseRecordingApp>(recordingApp);
                if (pulseApp->application == movedAppBinary)
                {
                    await(PulseApi::context_move_source_output_by_index(
                        context, pulseApp->id, originalSink,
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
        }
        movedApplications.clear();

        return success;
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
                    await(PulseApi::context_move_sink_input_by_index(context, pulsePlaybackApp->id, pulseOriginal->sink,
                                                                     nullptr, nullptr));
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
                    await(PulseApi::context_move_source_output_by_index(context, pulseRecordingApp->id,
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

        await(PulseApi::context_set_sink_input_mute(
            context, *defaultSourceId, state,
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

    bool PulseAudio::isRunningPipeWire()
    {
        //* The stuff we do here does is broken on pipewire-pulse, use the native backend instead.
        if (serverName.find("PipeWire") != std::string::npos)
        {
            Fancy::fancy.logTime().message()
                << "Detected PipeWire-Pulse, please use the native pipewire backend" << std::endl;
            return true;
        }

        return false;
    }

    std::set<std::string> PulseAudio::currentlyInputApps()
    {
        std::set<std::string> rtn;
        for (const auto &[app, original] : movedApplications)
        {
            rtn.emplace(app);
        }

        return rtn;
    }
    std::set<std::string> PulseAudio::currentlyPassedThrough()
    {
        std::set<std::string> rtn;
        for (const auto &[app, original] : movedPassthroughApplications)
        {
            rtn.emplace(app);
        }

        return rtn;
    }
} // namespace Soundux::Objects
#endif