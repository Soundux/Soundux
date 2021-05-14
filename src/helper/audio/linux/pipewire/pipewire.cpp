#if defined(__linux__)
#include "pipewire.hpp"
#include <fancy.hpp>
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
                const auto *rawAlias = spa_dict_lookup(props, "port.alias");
                const auto *rawPortName = spa_dict_lookup(props, "port.name");

                if (rawAlias && rawPortName)
                {
                    //* This is the only reliable way to get the name
                    //* PW_KEY_APP_NAME or PW_KEY_APP_PROCESS_BINARY are almost certainly never set.

                    std::string alias(rawAlias);
                    std::string portName(rawPortName);

                    auto name = std::string(alias);
                    name = name.substr(0, name.find_first_of(':'));

                    Direction direction;
                    if (portName.back() == 'R' || portName.back() == '2')
                    {
                        direction = Direction::FrontRight;
                    }
                    else if (portName.back() == 'L' || portName.back() == '1')
                    {
                        direction = Direction::FrontLeft;
                    }

                    if (name == "soundux_sink")
                    {
                        if (portName.find("playback") != std::string::npos)
                        {
                            if (direction == Direction::FrontRight)
                            {
                                thiz->nullSinkPlaybackRight = id;
                            }
                            else if (direction == Direction::FrontLeft)
                            {
                                thiz->nullSinkPlaybackLeft = id;
                            }
                        }
                        else
                        {
                            if (direction == Direction::FrontRight)
                            {
                                thiz->nullSinkRight = id;
                            }
                            else if (direction == Direction::FrontLeft)
                            {
                                thiz->nullSinkLeft = id;
                            }
                        }

                        return;
                    }

                    if (portName.find("output") != std::string::npos)
                    {
                        auto outputApp = std::make_shared<PipeWirePlaybackApp>();

                        outputApp->id = id;
                        outputApp->name = name;
                        outputApp->application = name;
                        outputApp->direction = direction;

                        std::lock_guard lock(thiz->playbackMutex);
                        thiz->playbackApps.emplace_back(outputApp);
                    }
                    else if (portName.find("input") != std::string::npos)
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
        createNullSink();
    }

    void PipeWire::destroy()
    {
        pw_proxy_destroy(reinterpret_cast<pw_proxy *>(registry));
        pw_core_disconnect(core);
        pw_context_destroy(context);
        pw_main_loop_destroy(loop);
    }

    bool PipeWire::createNullSink()
    {
        pw_properties *props = pw_properties_new(nullptr, nullptr);

        pw_properties_set(props, PW_KEY_MEDIA_CLASS, "Audio/Sink");
        pw_properties_set(props, PW_KEY_NODE_NAME, "soundux_sink");
        pw_properties_set(props, PW_KEY_FACTORY_NAME, "support.null-audio-sink");

        auto *proxy = reinterpret_cast<pw_proxy *>(
            pw_core_create_object(core, "adapter", PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, &props->dict, 0));

        if (!proxy)
        {
            pw_properties_free(props);
            return false;
        }

        spa_hook listener;
        bool success = false;
        pw_proxy_events linkEvent = {};
        linkEvent.version = PW_VERSION_PROXY_EVENTS;
        linkEvent.bound = [](void *data, [[maybe_unused]] std::uint32_t id) { *reinterpret_cast<bool *>(data) = true; };
        linkEvent.error = [](void *data, [[maybe_unused]] int a, [[maybe_unused]] int b, const char *message) {
            Fancy::fancy.logTime().failure() << "Failed to create null sink: " << message << std::endl;
            *reinterpret_cast<bool *>(data) = false;
        };

        spa_zero(listener);
        pw_proxy_add_listener(proxy, &listener, &linkEvent, &success);

        sync();

        spa_hook_remove(&listener);
        pw_properties_free(props);
        // pw_proxy_destroy(proxy); Not required.

        return success;
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

        //* By not setting linger to true we don't have to worry about unloading left overs!
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
            Fancy::fancy.logTime().failure() << "Failed to create link: " << message << std::endl;
            *reinterpret_cast<std::optional<std::uint32_t> *>(data) = std::nullopt;
        };

        spa_zero(listener);
        pw_proxy_add_listener(proxy, &listener, &linkEvent, &result);

        sync();

        spa_hook_remove(&listener);
        pw_properties_free(props);
        // pw_proxy_destroy(proxy); Not required

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
        Fancy::fancy.logTime().warning() << "Fix Me: useAsDefault() is not yet implemented on pipewire" << std::endl;
        return false;
    }

    bool PipeWire::revertDefault()
    {
        // TODO(pipewire): Delete link created by `useAsDefault`
        Fancy::fancy.logTime().warning() << "Fix Me: revertDefault() is not yet implemented on pipewire" << std::endl;
        return true;
    }

    bool PipeWire::muteInput(bool state)
    {
        // TODO(pipewire): Maybe we could delete any link from the microphone to the output app and recreate it?
        Fancy::fancy.logTime().warning() << "Fix Me: muteInput() is not yet implemented on pipewire" << std::endl;

        (void)state;
        return false;
    }

    bool PipeWire::inputSoundTo(std::shared_ptr<RecordingApp> app)
    {
        if (!app)
        {
            return false;
        }

        stopSoundInput();
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

        bool success = true;

        for (const auto &pipeWireApp : toMove)
        {
            if (pipeWireApp.direction == Direction::FrontLeft)
            {
                auto linkId = linkPorts(pipeWireApp.id, nullSinkLeft);
                if (linkId)
                {
                    soundInputLinks.emplace_back(*linkId);
                }
                else
                {
                    success = false;
                }
            }
            else
            {
                auto linkId = linkPorts(pipeWireApp.id, nullSinkRight);
                if (linkId)
                {
                    soundInputLinks.emplace_back(*linkId);
                }
                else
                {
                    success = false;
                }
            }
        }

        return success;
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
        if (!app)
        {
            return false;
        }

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

        bool success = true;
        for (const auto &pipeWireApp : toMove)
        {
            if (pipeWireApp.direction == Direction::FrontLeft)
            {
                auto linkId = linkPorts(nullSinkPlaybackLeft, pipeWireApp.id);
                if (linkId)
                {
                    passthroughLinks.emplace_back(*linkId);
                }
                else
                {
                    success = false;
                }
            }
            else
            {
                auto linkId = linkPorts(nullSinkPlaybackRight, pipeWireApp.id);
                if (linkId)
                {
                    passthroughLinks.emplace_back(*linkId);
                }
                else
                {
                    success = false;
                }
            }
        }

        return success;
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
#endif