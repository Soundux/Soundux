#include "pipewire.hpp"
#include <memory>
#include <optional>
#include <stdexcept>

namespace Soundux::Objects
{
    void PipeWire::sync()
    {
        spa_hook coreListener;
        int pending = 0;

        pw_core_events coreEvents = {};
        coreEvents.version = PW_VERSION_CORE_EVENTS;
        coreEvents.done = [](void *data, uint32_t id, int seq) {
            auto *info = reinterpret_cast<std::pair<PipeWire *, int *> *>(data);
            if (info)
            {
                if (id == PW_ID_CORE && seq == *info->second)
                {
                    *info->second = -1;
                    pw_main_loop_quit(info->first->loop);
                }
            }
        };

        spa_zero(coreListener);
        auto data = std::make_pair(this, &pending);
        pw_core_add_listener(core, &coreListener, &coreEvents, &data); // NOLINT

        pending = pw_core_sync(core, PW_ID_CORE, 0); // NOLINT
        while (pending != -1)
        {
            pw_main_loop_run(loop);
        }

        spa_hook_remove(&coreListener);
    }

    void PipeWire::onGlobalAdded(void *data, std::uint32_t id, [[maybe_unused]] std::uint32_t perms, const char *type,
                                 [[maybe_unused]] std::uint32_t version, const spa_dict *props)
    {
        auto *thiz = reinterpret_cast<PipeWire *>(data);
        if (thiz && data && props && type)
        {
            if (strcmp(type, PW_TYPE_INTERFACE_Port) == 0)
            {
                const auto *alias = spa_dict_lookup(props, "port.alias");
                const auto *portName = spa_dict_lookup(props, "port.name");

                if (alias && portName)
                {
                    //* This is the only reliable way to get the name
                    //* PW_KEY_APP_NAME or PW_KEY_APP_PROCESS_BINARY are almost certainly never set.
                    auto name = std::string(alias);
                    name = name.substr(0, name.find_first_of(':'));

                    Direction direction;
                    if (strstr(portName, "FR"))
                    {
                        direction = Direction::FrontRight;
                    }
                    else
                    {
                        direction = Direction::FrontLeft;
                    }

                    if (strstr(portName, "output"))
                    {
                        auto outputApp = std::make_shared<PipeWirePlaybackApp>();

                        outputApp->id = id;
                        outputApp->name = name;
                        outputApp->application = name;
                        outputApp->direction = direction;

                        std::lock_guard lock(thiz->playbackMutex);
                        thiz->playbackApps.emplace_back(outputApp);
                    }
                    else if (strstr(portName, "input"))
                    {
                        auto recordingApp = std::make_shared<PipeWireRecordingApp>();

                        recordingApp->id = id;
                        recordingApp->name = name;
                        recordingApp->application = name;
                        recordingApp->direction = direction;

                        std::lock_guard lock(thiz->recordingMutex);
                        thiz->recordingApps.emplace_back(recordingApp);
                    }
                }
            }
            else if (strcmp(type, PW_TYPE_INTERFACE_Link) == 0)
            {
                //* Should be used later to delete left over Soundux Links.
                const auto *inputPort = spa_dict_lookup(props, "link.input.port");
                const auto *outputPort = spa_dict_lookup(props, "link.output.port");

                if (inputPort && outputPort)
                {
                    const auto in = std::stol(inputPort);
                    const auto out = std::stol(outputPort);
                    std::lock_guard lock(thiz->playbackMutex);

                    for (auto &app : thiz->playbackApps)
                    {
                        auto pipeWireApp = std::dynamic_pointer_cast<PipeWirePlaybackApp>(app);

                        if (pipeWireApp->id == out)
                        {
                            pipeWireApp->links.emplace_back(Link{static_cast<std::uint32_t>(in)});
                        }
                    }
                }
            }
        }
    }

    void PipeWire::onGlobalRemoved(void *data, std::uint32_t id)
    {
        auto *thiz = reinterpret_cast<PipeWire *>(data);

        if (thiz)
        {
            thiz->playbackMutex.lock();
            for (auto it = thiz->playbackApps.begin(); it != thiz->playbackApps.end();)
            {
                auto pipeWireApp = std::dynamic_pointer_cast<PipeWirePlaybackApp>(*it);
                if (pipeWireApp && pipeWireApp->id == id)
                {
                    it = thiz->playbackApps.erase(it);
                }
                else
                {
                    it++;
                }
            }
            thiz->playbackMutex.unlock();

            thiz->recordingMutex.lock();
            for (auto it = thiz->recordingApps.begin(); it != thiz->recordingApps.end();)
            {
                auto pipeWireApp = std::dynamic_pointer_cast<PipeWireRecordingApp>(*it);
                if (pipeWireApp && pipeWireApp->id == id)
                {
                    it = thiz->recordingApps.erase(it);
                }
                else
                {
                    it++;
                }
            }
            thiz->recordingMutex.unlock();
        }
    }

    void PipeWire::setup()
    {
        pw_init(nullptr, nullptr);
        loop = pw_main_loop_new(nullptr);
        if (!loop)
        {
            throw std::runtime_error("Failed to create main loop");
        }
        context = pw_context_new(pw_main_loop_get_loop(loop), nullptr, 0);
        if (!context)
        {
            throw std::runtime_error("Failed to create context");
        }

        core = pw_context_connect(context, nullptr, 0);
        if (!core)
        {
            throw std::runtime_error("Failed to connect context");
        }
        registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
        if (!registry)
        {
            throw std::runtime_error("Failed to get registry");
        }

        spa_zero(registryListener);
        registryEvents.global = onGlobalAdded;
        registryEvents.global_remove = onGlobalRemoved;
        registryEvents.version = PW_VERSION_REGISTRY_EVENTS;

        pw_registry_add_listener(registry, &registryListener, &registryEvents, this); // NOLINT
        sync();
    }

    void PipeWire::destroy()
    {
        pw_proxy_destroy(reinterpret_cast<pw_proxy *>(registry));
        pw_core_disconnect(core);
        pw_context_destroy(context);
        pw_main_loop_destroy(loop);
    }

    bool PipeWire::deleteLink(std::uint32_t id)
    {
        pw_registry_destroy(registry, id); // NOLINT
        sync();

        return true;
    }

    std::optional<int> PipeWire::linkPorts(std::uint32_t in, std::uint32_t out)
    {
        pw_properties *props = pw_properties_new(nullptr, nullptr);

        pw_properties_set(props, PW_KEY_APP_NAME, "soundux");
        // pw_properties_set(props, PW_KEY_OBJECT_LINGER, "true");
        pw_properties_setf(props, PW_KEY_LINK_INPUT_PORT, "%u", in);
        pw_properties_setf(props, PW_KEY_LINK_OUTPUT_PORT, "%u", out);

        auto *proxy = reinterpret_cast<pw_proxy *>(
            pw_core_create_object(core, "link-factory", PW_TYPE_INTERFACE_Link, PW_VERSION_LINK, &props->dict, 0));

        if (!proxy)
        {
            pw_properties_free(props);
            return std::nullopt;
        }

        spa_hook listener;
        std::optional<std::uint32_t> result;

        pw_proxy_events linkEvent = {};
        linkEvent.version = PW_VERSION_PROXY_EVENTS;
        linkEvent.bound = [](void *data, std::uint32_t id) {
            *reinterpret_cast<std::optional<std::uint32_t> *>(data) = id;
        };
        linkEvent.error = [](void *data, [[maybe_unused]] int a, [[maybe_unused]] int b, const char *message) {
            printf("Error: %s\n", message);
            *reinterpret_cast<std::optional<std::uint32_t> *>(data) = std::nullopt;
        };

        spa_zero(listener);
        pw_proxy_add_listener(proxy, &listener, &linkEvent, &result);

        sync();

        spa_hook_remove(&listener);
        pw_properties_free(props);
        pw_proxy_destroy(proxy);

        return result;
    }

    std::vector<std::shared_ptr<RecordingApp>> PipeWire::getRecordingApps()
    {
        sync();
        return recordingApps;
    }

    std::vector<std::shared_ptr<PlaybackApp>> PipeWire::getPlaybackApps()
    {
        sync();
        std::lock_guard lock(playbackMutex);
        std::vector<std::shared_ptr<PlaybackApp>> rtn;

        for (const auto &app : playbackApps)
        {
            auto pipeWireApp = std::dynamic_pointer_cast<PipeWirePlaybackApp>(app);
            if (!pipeWireApp->links.empty())
            {
                rtn.emplace_back(app);
            }
        }

        return rtn;
    }

    std::shared_ptr<PlaybackApp> PipeWire::getPlaybackApp(const std::string &name)
    {
        std::lock_guard lock(playbackMutex);
        for (const auto &app : playbackApps)
        {
            if (app->name == name)
            {
                return app;
            }
        }

        return nullptr;
    }

    std::shared_ptr<RecordingApp> PipeWire::getRecordingApp(const std::string &name)
    {
        std::lock_guard lock(recordingMutex);
        for (const auto &app : recordingApps)
        {
            if (app->name == name)
            {
                return app;
            }
        }

        return nullptr;
    }

    bool PipeWire::useAsDefault()
    {
        // TODO(pipewire): Find a way to connect the output to the microphone
        return false;
    }

    bool PipeWire::revertDefault()
    {
        // TODO(pipewire): Delete link created by `useAsDefault`
        return true;
    }

    bool PipeWire::muteInput(bool state)
    {
        // TODO(pipewire): Maybe we could delete any link from the microphone to the output app and recreate it?
        (void)state;
        return false;
    }

    bool PipeWire::inputSoundTo(std::shared_ptr<RecordingApp> app)
    {
        std::vector<PipeWireRecordingApp> toMove;

        recordingMutex.lock();
        for (const auto &recordingApp : recordingApps)
        {
            if (recordingApp->name == app->name)
            {
                auto pipeWireApp = std::dynamic_pointer_cast<PipeWireRecordingApp>(recordingApp);
                toMove.emplace_back(*pipeWireApp);
            }
        }
        recordingMutex.unlock();

        for (const auto &pipeWireApp : toMove)
        {
            (void)pipeWireApp;
            // TODO(pipewire): Link null sink to each app
            // TODO(pipewire): Save id of each created link
        }

        return true;
    }

    bool PipeWire::stopSoundInput()
    {
        for (const auto &id : soundInputLinks)
        {
            deleteLink(id);
        }
        soundInputLinks.clear();

        return true;
    }

    bool PipeWire::passthroughFrom(std::shared_ptr<PlaybackApp> app)
    {
        std::vector<PipeWirePlaybackApp> toMove;

        playbackMutex.lock();
        for (const auto &playbackApp : playbackApps)
        {
            if (playbackApp->name == app->name)
            {
                auto pipeWireApp = std::dynamic_pointer_cast<PipeWirePlaybackApp>(playbackApp);
                toMove.emplace_back(*pipeWireApp);
            }
        }
        playbackMutex.unlock();

        for (const auto &pipeWireApp : toMove)
        {
            (void)pipeWireApp;
            // TODO(pipewire): Link each app to null sink
            // TODO(pipewire): Save id of each created link
        }

        return true;
    }

    bool PipeWire::isCurrentlyPassingThrough()
    {
        return !passthroughLinks.empty();
    }

    bool PipeWire::stopPassthrough()
    {
        for (const auto &id : passthroughLinks)
        {
            deleteLink(id);
        }
        passthroughLinks.clear();

        return true;
    }
} // namespace Soundux::Objects