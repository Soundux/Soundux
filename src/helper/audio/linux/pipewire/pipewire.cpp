#if defined(__linux__)
#include "pipewire.hpp"
#include "forward.hpp"
#include <fancy.hpp>
#include <memory>
#include <nlohmann/json.hpp>
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
                    PipeWireApi::main_loop_quit(info->first->loop);
                }
            }
        };
        coreEvents.error = [](void *data, std::uint32_t id, int seq, int res, const char *message) {
            auto *info = reinterpret_cast<std::pair<PipeWire *, int *> *>(data);
            if (info)
            {
                if (id == PW_ID_CORE && seq == *info->second)
                {
                    Fancy::fancy.logTime()
                        << "Core Failure - Seq " << seq << " - Res " << res << ": " << message << std::endl;

                    *info->second = -1;
                    PipeWireApi::main_loop_quit(info->first->loop);
                }
            }
        };

        auto data = std::make_pair(this, &pending);
        pw_core_add_listener(core, &coreListener, &coreEvents, &data); // NOLINT

        pending = pw_core_sync(core, PW_ID_CORE, 0); // NOLINT
        while (pending != -1)
        {
            PipeWireApi::main_loop_run(loop);
        }

        spa_hook_remove(&coreListener);
    }

    void PipeWire::onNodeInfo(const pw_node_info *info)
    {
        auto &nodes = this->nodes.unsafe();
        if (info && nodes.find(info->id) != nodes.end())
        {
            auto &self = nodes.at(info->id);
            self.inputPorts = info->n_input_ports;
            self.outputPorts = info->n_output_ports;

            if (const auto *pid = spa_dict_lookup(info->props, "application.process.id"); pid)
            {
                self.pid = std::stol(pid);
            }
            if (const auto *monitor = spa_dict_lookup(info->props, "stream.monitor"); monitor)
            {
                self.isMonitor = true;
            }

            if (const auto *appName = spa_dict_lookup(info->props, "application.name"); appName)
            {
                self.name = appName;
            }
            if (const auto *binary = spa_dict_lookup(info->props, "application.process.binary"); binary)
            {
                self.applicationBinary = binary;
            }
        }
    }

    void PipeWire::onCoreInfo(const pw_core_info *info)
    {
        if (info && info->name && info->version && !version)
        {
            Fancy::fancy.logTime().message()
                << "Connected to PipeWire (" << info->name << ") on version " << info->version << std::endl;

            std::string formattedVersion(info->version);
            formattedVersion.erase(std::remove(formattedVersion.begin(), formattedVersion.end(), '.'),
                                   formattedVersion.end());

            version = std::stoi(formattedVersion);
            if (version < 326)
            {
                Fancy::fancy.logTime().warning() << "Your PipeWire version is below the minimum required (0.3.26), "
                                                    "you may experience bugs or crashes"
                                                 << std::endl;
            }
        }
    }

    void PipeWire::onGlobalAdded(void *data, std::uint32_t id, [[maybe_unused]] std::uint32_t perms, const char *type,
                                 [[maybe_unused]] std::uint32_t version, const spa_dict *props)
    {
        auto *thiz = reinterpret_cast<PipeWire *>(data);
        if (thiz && props)
        {
            if (strcmp(type, PW_TYPE_INTERFACE_Core) == 0)
            {
                spa_hook listener;
                pw_core_events events = {};

                events.info = [](void *data, const pw_core_info *info) {
                    auto *thiz = reinterpret_cast<PipeWire *>(data);
                    if (thiz)
                    {
                        thiz->onCoreInfo(info);
                    }
                };
                events.version = PW_VERSION_CORE_EVENTS;
                auto *boundCore = reinterpret_cast<pw_core *>(
                    pw_registry_bind(thiz->registry, id, type, PW_VERSION_CORE, sizeof(PipeWire)));

                if (boundCore)
                {
                    pw_core_add_listener(boundCore, &listener, &events, thiz); // NOLINT
                    thiz->sync();
                    spa_hook_remove(&listener);
                    PipeWireApi::proxy_destroy(reinterpret_cast<pw_proxy *>(boundCore));
                }
            }
            if (strcmp(type, PW_TYPE_INTERFACE_Metadata) == 0)
            {
                spa_hook listener;
                pw_metadata_events events = {};

                events.property = [](void *userdata, [[maybe_unused]] std::uint32_t id, const char *key,
                                     [[maybe_unused]] const char *type, const char *value) -> int {
                    auto *thiz = reinterpret_cast<PipeWire *>(userdata);
                    if (thiz && key && value)
                    {
                        if (strcmp(key, "default.audio.source") == 0)
                        {
                            auto parsedValue = nlohmann::json::parse(value, nullptr, false);
                            if (!parsedValue.is_discarded() && parsedValue.count("name"))
                            {
                                auto name = parsedValue["name"].get<std::string>();

                                thiz->defaultMicrophone = name;
                                Fancy::fancy.logTime().message() << "Found default device: " << name << std::endl;
                            }
                        }
                    }
                    return 0;
                };

                auto *boundMetaData = reinterpret_cast<pw_metadata *>(
                    pw_registry_bind(thiz->registry, id, type, PW_VERSION_METADATA, sizeof(PipeWire)));

                if (boundMetaData)
                {
                    pw_metadata_add_listener(boundMetaData, &listener, &events, thiz); // NOLINT
                    thiz->sync();
                    spa_hook_remove(&listener);
                    PipeWireApi::proxy_destroy(reinterpret_cast<pw_proxy *>(boundMetaData));
                }
            }
            if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0)
            {
                const auto *name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
                if (name && strcmp(name, "soundux") == 0)
                {
                    return;
                }

                Node node;
                node.id = id;
                node.rawName = name;

                spa_hook listener;
                pw_node_events events = {};

                events.info = [](void *data, const pw_node_info *info) {
                    auto *thiz = reinterpret_cast<PipeWire *>(data);
                    if (thiz)
                    {
                        thiz->onNodeInfo(info);
                    }
                };
                events.version = PW_VERSION_NODE_EVENTS;
                auto *boundNode = reinterpret_cast<pw_node *>(
                    pw_registry_bind(thiz->registry, id, type, PW_VERSION_NODE, sizeof(PipeWire)));

                if (boundNode)
                {
                    //* We lock the mutex here, because we only need access to `nodes` as soon as sync is called.
                    thiz->nodes.write()->emplace(id, node);
                    pw_node_add_listener(boundNode, &listener, &events, thiz); // NOLINT
                    thiz->sync();
                    spa_hook_remove(&listener);
                    PipeWireApi::proxy_destroy(reinterpret_cast<pw_proxy *>(boundNode));
                }
            }
        }
    }

    void PipeWire::onGlobalRemoved(void *data, std::uint32_t id)
    {
        auto *thiz = reinterpret_cast<PipeWire *>(data);
        if (thiz)
        {
            auto nodes = thiz->nodes.write();
            if (nodes->find(id) != nodes->end())
            {
                nodes->erase(id);
            }
        }
    }

    bool PipeWire::setup()
    {
        if (!PipeWireApi::setup())
        {
            return false;
        }

        PipeWireApi::init(nullptr, nullptr);
        loop = PipeWireApi::main_loop_new(nullptr);
        if (!loop)
        {
            Fancy::fancy.logTime().failure() << "Failed to create main loop" << std::endl;
            return false;
        }
        context = PipeWireApi::context_new(PipeWireApi::main_loop_get_loop(loop), nullptr, 0);
        if (!context)
        {
            Fancy::fancy.logTime().failure() << "Failed to create context" << std::endl;
            return false;
        }
        core = PipeWireApi::context_connect(context, nullptr, 0);
        if (!core)
        {
            Fancy::fancy.logTime().failure() << "Failed to connect context" << std::endl;
            return false;
        }
        registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
        if (!registry)
        {
            Fancy::fancy.logTime().failure() << "Failed to get registry" << std::endl;
            return false;
        }

        registryEvents.global = onGlobalAdded;
        registryEvents.global_remove = onGlobalRemoved;
        registryEvents.version = PW_VERSION_REGISTRY_EVENTS;

        pw_registry_add_listener(registry, &registryListener, &registryEvents, this); // NOLINT

        sync();

        if (defaultMicrophone.empty())
        {
            Fancy::fancy.logTime().warning() << "Failed to retrieve default microphone" << std::endl;
        }

        return createNullSink();
    }

    void PipeWire::destroy()
    {
        PipeWireApi::proxy_destroy(reinterpret_cast<pw_proxy *>(registry));
        PipeWireApi::core_disconnect(core);
        PipeWireApi::context_destroy(context);
        PipeWireApi::main_loop_destroy(loop);
    }

    bool PipeWire::createNullSink()
    {
        pw_properties *props = PipeWireApi::properties_new(nullptr, nullptr);

        PipeWireApi::properties_set(props, PW_KEY_MEDIA_CLASS, "Audio/Sink");
        PipeWireApi::properties_set(props, PW_KEY_NODE_NAME, "soundux_sink");
        PipeWireApi::properties_set(props, PW_KEY_FACTORY_NAME, "support.null-audio-sink");

        auto *proxy = reinterpret_cast<pw_proxy *>(
            pw_core_create_object(core, "adapter", PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, &props->dict, 0));

        if (!proxy)
        {
            Fancy::fancy.logTime().failure() << "Failed to create null sink node" << std::endl;
            PipeWireApi::properties_free(props);
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

        PipeWireApi::proxy_add_listener(proxy, &listener, &linkEvent, &success);
        sync();

        spa_hook_remove(&listener);
        PipeWireApi::properties_free(props);

        return success;
    }

    bool PipeWire::deleteLink(std::uint32_t id)
    {
        pw_registry_destroy(registry, id); // NOLINT
        sync();

        return true;
    }

    std::optional<int> PipeWire::createLink(std::uint32_t in, std::uint32_t out)
    {
        pw_properties *props = PipeWireApi::properties_new(nullptr, nullptr);

        PipeWireApi::properties_set(props, PW_KEY_APP_NAME, "soundux");
        PipeWireApi::properties_setf(props, PW_KEY_LINK_INPUT_PORT, "%i", -1);
        PipeWireApi::properties_setf(props, PW_KEY_LINK_OUTPUT_PORT, "%i", -1);
        PipeWireApi::properties_set(props, PW_KEY_LINK_INPUT_NODE, std::to_string(in).c_str());
        PipeWireApi::properties_set(props, PW_KEY_LINK_OUTPUT_NODE, std::to_string(out).c_str());

        auto *proxy = reinterpret_cast<pw_proxy *>(
            pw_core_create_object(core, "link-factory", PW_TYPE_INTERFACE_Link, PW_VERSION_LINK, &props->dict, 0));

        if (!proxy)
        {
            Fancy::fancy.logTime().warning() << "Failed to create link from " << in << " to " << out << std::endl;
            PipeWireApi::properties_free(props);
            return std::nullopt;
        }

        spa_hook listener;
        std::optional<std::uint32_t> result;

        pw_proxy_events linkEvent = {};
        linkEvent.version = PW_VERSION_PROXY_EVENTS;
        linkEvent.bound = [](void *data, std::uint32_t id) {
            *reinterpret_cast<std::optional<std::uint32_t> *>(data) = id;
        };
        linkEvent.error = []([[maybe_unused]] void *data, int seq, int res, const char *message) {
            Fancy::fancy.logTime().warning()
                << "An error occurred while creating a link(" << seq << ":" << res << "): " << message << std::endl;
        };

        PipeWireApi::proxy_add_listener(proxy, &listener, &linkEvent, &result);
        sync();

        spa_hook_remove(&listener);
        PipeWireApi::properties_free(props);

        return result;
    }

    std::vector<std::shared_ptr<RecordingApp>> PipeWire::getRecordingApps()
    {
        sync();
        std::vector<std::shared_ptr<RecordingApp>> rtn;

        auto nodes = this->nodes.read();
        for (const auto &[nodeId, node] : *nodes)
        {
            if (!node.name.empty() && !node.isMonitor)
            {
                if (node.inputPorts)
                {
                    PipeWireRecordingApp app;
                    app.node = node;
                    app.name = node.name;
                    app.application = node.applicationBinary;
                    rtn.emplace_back(std::make_shared<PipeWireRecordingApp>(app));
                }
            }
        }

        return rtn;
    }

    std::vector<std::shared_ptr<PlaybackApp>> PipeWire::getPlaybackApps()
    {
        sync();
        std::vector<std::shared_ptr<PlaybackApp>> rtn;

        auto nodes = this->nodes.read();
        for (const auto &[nodeId, node] : *nodes)
        {
            if (!node.name.empty() && !node.isMonitor)
            {
                if (node.outputPorts)
                {
                    PipeWirePlaybackApp app;
                    app.node = node;
                    app.name = node.name;
                    app.application = node.applicationBinary;
                    rtn.emplace_back(std::make_shared<PipeWirePlaybackApp>(app));
                }
            }
        }

        return rtn;
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
        // TODO(pipewire): Research if it's possible to mute the device instead of the node.
        auto copy = nodes.copy();
        for (const auto &node : copy)
        {
            if (node.second.rawName == defaultMicrophone)
            {
                auto *boundNode = reinterpret_cast<pw_node *>(pw_registry_bind(
                    registry, node.second.id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, sizeof(PipeWire)));

                if (boundNode)
                {
                    char buffer[1024];
                    spa_pod_builder b;
                    spa_pod_builder_init(&b, buffer, sizeof(buffer));

                    spa_pod_frame f[1];
                    spa_pod *param{};

                    spa_pod_builder_push_object(&b, &f[0], SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
                    spa_pod_builder_add(&b, SPA_PROP_mute, SPA_POD_Bool(state), 0);

                    param = static_cast<spa_pod *>(spa_pod_builder_pop(&b, &f[0]));
                    if (pw_node_set_param(boundNode, SPA_PARAM_Props, 0, param) < 0) // NOLINT
                    {
                        return false;
                    }

                    PipeWireApi::proxy_destroy(reinterpret_cast<pw_proxy *>(boundNode));
                    sync();

                    return true;
                }

                break;
            }
        }

        return false;
    }

    bool PipeWire::inputSoundTo(const std::string &app)
    {
        sync();

        if (app.empty())
        {
            Fancy::fancy.logTime().warning() << "Invalid app" << std::endl;
            return false;
        }

        if (soundInputLinks.count(app))
        {
            return true;
        }

        bool success = false;
        auto nodes = this->nodes.copy();
        std::vector<std::uint32_t> createdLinks;

        for (const auto &[nodeId, node] : nodes)
        {
            if (node.rawName == "soundux_sink")
            {
                for (const auto &[appNodeId, appNode] : nodes)
                {
                    if (appNode.applicationBinary == app && appNode.inputPorts)
                    {
                        auto link = createLink(appNodeId, nodeId);

                        if (link)
                        {
                            success = true;
                            createdLinks.emplace_back(*link);
                        }
                    }
                }
            }
        }

        if (success)
        {
            soundInputLinks.emplace(app, createdLinks);
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Could not link app " << app << std::endl;
        }

        return success;
    }

    bool PipeWire::stopSoundInput()
    {
        for (const auto &[appBinary, links] : soundInputLinks)
        {
            for (const auto &id : links)
            {
                deleteLink(id);
            }
        }
        soundInputLinks.clear();

        return true;
    }

    bool PipeWire::passthroughFrom(const std::string &app)
    {
        sync();

        if (app.empty())
        {
            Fancy::fancy.logTime().warning() << "Invalid app" << std::endl;
            return false;
        }

        bool success = false;
        auto nodes = this->nodes.copy();
        std::vector<std::uint32_t> createdLinks;

        for (const auto &[nodeId, node] : nodes)
        {
            if (node.rawName == "soundux_sink")
            {
                for (const auto &[appNodeId, appNode] : nodes)
                {
                    if (appNode.applicationBinary == app && appNode.outputPorts)
                    {
                        auto link = createLink(nodeId, appNodeId);

                        if (link)
                        {
                            success = true;
                            createdLinks.emplace_back(*link);
                        }
                    }
                }
            }
        }

        if (success)
        {
            passthroughLinks.emplace(app, createdLinks);
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Could not link app " << app << std::endl;
        }

        return success;
    }

    std::set<std::string> PipeWire::currentlyInputApps()
    {
        std::set<std::string> rtn;
        for (const auto &[app, links] : soundInputLinks)
        {
            rtn.emplace(app);
        }

        return rtn;
    }
    std::set<std::string> PipeWire::currentlyPassedThrough()
    {
        std::set<std::string> rtn;
        for (const auto &[app, links] : passthroughLinks)
        {
            rtn.emplace(app);
        }

        return rtn;
    }

    bool PipeWire::stopPassthrough(const std::string &app)
    {
        if (passthroughLinks.find(app) != passthroughLinks.end())
        {
            for (const auto &id : passthroughLinks.at(app))
            {
                deleteLink(id);
            }

            passthroughLinks.erase(app);
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Could not find links for application " << app << std::endl;
        }

        return true;
    }

    bool PipeWire::stopAllPassthrough()
    {
        for (const auto &[appBinary, links] : passthroughLinks)
        {
            for (const auto &id : links)
            {
                deleteLink(id);
            }
        }

        passthroughLinks.clear();
        return true;
    }
} // namespace Soundux::Objects
#endif