#if defined(__linux__)
#include "pipewire.hpp"
#include "forward.hpp"
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
        if (info && (~nodes).find(info->id) != (~nodes).end())
        {
            auto &self = (~nodes).at(info->id);

            if (const auto *pid = spa_dict_lookup(info->props, "application.process.id"); pid)
            {
                self.pid = std::stol(pid);
            }
            if (const auto *monitor = spa_dict_lookup(info->props, "stream.monitor"); monitor)
            {
                self.isMonitor = true;
            }

            //* Yes this is swapped. (For compatibility reasons)
            if (const auto *appName = spa_dict_lookup(info->props, "application.name"); appName)
            {
                self.applicationBinary = appName;
            }
            if (const auto *binary = spa_dict_lookup(info->props, "application.process.binary"); binary)
            {
                self.name = binary;
            }
        }
    }

    void PipeWire::onPortInfo(const pw_port_info *info)
    {
        if (info && (~ports).find(info->id) != (~ports).end())
        {
            auto &self = (~ports).at(info->id);
            self.direction = info->direction;

            if (const auto *nodeId = spa_dict_lookup(info->props, "node.id"); nodeId)
            {
                self.parentNode = std::stol(nodeId);
            }
            if (const auto *portName = spa_dict_lookup(info->props, "port.name"); portName)
            {
                self.side = std::string(portName).back();
            }
            if (const auto *portAlias = spa_dict_lookup(info->props, "port.alias"); portAlias)
            {
                self.portAlias = std::string(portAlias);
            }
        }
    }

    void PipeWire::onGlobalAdded(void *data, std::uint32_t id, [[maybe_unused]] std::uint32_t perms, const char *type,
                                 [[maybe_unused]] std::uint32_t version, const spa_dict *props)
    {
        auto *thiz = reinterpret_cast<PipeWire *>(data);
        if (thiz && props)
        {
            if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0)
            {
                const auto *name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
                if (name && strstr(name, "soundux"))
                {
                    return;
                }

                Node node;
                node.id = id;

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
                    thiz->nodes->emplace(id, node);
                    pw_node_add_listener(boundNode, &listener, &events, thiz); // NOLINT
                    thiz->sync();
                    spa_hook_remove(&listener);
                    PipeWireApi::proxy_destroy(reinterpret_cast<pw_proxy *>(boundNode));
                }
            }
            if (strcmp(type, PW_TYPE_INTERFACE_Port) == 0)
            {
                Port port;
                port.id = id;

                spa_hook listener;
                pw_port_events events = {};

                events.info = [](void *data, const pw_port_info *info) {
                    auto *thiz = reinterpret_cast<PipeWire *>(data);
                    if (thiz)
                    {
                        thiz->onPortInfo(info);
                    }
                };
                events.version = PW_VERSION_PORT_EVENTS;

                auto *boundPort =
                    reinterpret_cast<pw_port *>(pw_registry_bind(thiz->registry, id, type, version, sizeof(PipeWire)));

                if (boundPort)
                {
                    thiz->ports->emplace(id, port);
                    pw_port_add_listener(boundPort, &listener, &events, thiz); // NOLINT
                    thiz->sync();
                    spa_hook_remove(&listener);
                    PipeWireApi::proxy_destroy(reinterpret_cast<pw_proxy *>(boundPort));

                    auto scopedNodes = thiz->nodes.scoped();
                    auto scopedPorts = thiz->ports.scoped();
                    if (scopedPorts->find(id) != scopedPorts->end())
                    {
                        auto &port = scopedPorts->at(id);
                        if (port.parentNode > 0 && scopedNodes->find(port.parentNode) != scopedNodes->end())
                        {
                            auto &node = scopedNodes->at(port.parentNode);
                            node.ports.emplace(id, port);
                            scopedPorts->erase(id);
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
            auto scopedNodes = thiz->nodes.scoped();
            if (scopedNodes->find(id) != scopedNodes->end())
            {
                scopedNodes->erase(id);
            }

            auto scopedPorts = thiz->ports.scoped();
            if (scopedPorts->find(id) != scopedPorts->end())
            {
                scopedPorts->erase(id);
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

        if (!createNullSink())
        {
            Fancy::fancy.logTime().failure() << "Failed to create null sink" << std::endl;
            return false;
        }

        return true;
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

    std::optional<int> PipeWire::linkPorts(std::uint32_t in, std::uint32_t out)
    {
        pw_properties *props = PipeWireApi::properties_new(nullptr, nullptr);

        PipeWireApi::properties_set(props, PW_KEY_APP_NAME, "soundux");
        PipeWireApi::properties_setf(props, PW_KEY_LINK_INPUT_PORT, "%u", in);
        PipeWireApi::properties_setf(props, PW_KEY_LINK_OUTPUT_PORT, "%u", out);

        auto *proxy = reinterpret_cast<pw_proxy *>(
            pw_core_create_object(core, "link-factory", PW_TYPE_INTERFACE_Link, PW_VERSION_LINK, &props->dict, 0));

        if (!proxy)
        {
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
        linkEvent.error = [](void *data, [[maybe_unused]] int a, [[maybe_unused]] int b,
                             [[maybe_unused]] const char *message) {
            *reinterpret_cast<std::optional<std::uint32_t> *>(data) = std::nullopt;
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

        auto scopedNodes = nodes.scoped();
        for (const auto &[nodeId, node] : *scopedNodes)
        {
            if (!node.applicationBinary.empty() && !node.isMonitor)
            {
                bool hasInput = false;
                for (const auto &[portId, port] : node.ports)
                {
                    if (port.direction == SPA_DIRECTION_INPUT)
                    {
                        hasInput = true;
                        break;
                    }
                }

                if (hasInput)
                {
                    PipeWireRecordingApp app;
                    app.pid = node.pid;
                    app.nodeId = nodeId;
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

        auto scopedNodes = nodes.scoped();
        for (const auto &[nodeId, node] : *scopedNodes)
        {
            if (!node.applicationBinary.empty() && !node.isMonitor)
            {
                bool hasOutput = false;
                for (const auto &[portId, port] : node.ports)
                {
                    if (port.direction == SPA_DIRECTION_OUTPUT)
                    {
                        hasOutput = true;
                        break;
                    }
                }

                if (hasOutput)
                {
                    PipeWirePlaybackApp app;
                    app.pid = node.pid;
                    app.nodeId = nodeId;
                    app.name = node.name;
                    app.application = node.applicationBinary;
                    rtn.emplace_back(std::make_shared<PipeWirePlaybackApp>(app));
                }
            }
        }

        return rtn;
    }

    std::shared_ptr<PlaybackApp> PipeWire::getPlaybackApp(const std::string &name)
    {
        auto scopedNodes = nodes.scoped();
        for (const auto &[nodeId, node] : *scopedNodes)
        {
            if (node.name == name)
            {
                PipeWirePlaybackApp app;
                app.pid = node.pid;
                app.nodeId = nodeId;
                app.name = node.name;
                app.application = node.applicationBinary;
                return std::make_shared<PipeWirePlaybackApp>(app);
            }
        }

        return nullptr;
    }

    std::shared_ptr<RecordingApp> PipeWire::getRecordingApp(const std::string &name)
    {
        auto scopedNodes = nodes.scoped();
        for (const auto &[nodeId, node] : *scopedNodes)
        {
            if (node.name == name)
            {
                PipeWireRecordingApp app;
                app.pid = node.pid;
                app.nodeId = nodeId;
                app.name = node.name;
                app.application = node.applicationBinary;
                return std::make_shared<PipeWireRecordingApp>(app);
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

        auto pipeWireApp = std::dynamic_pointer_cast<PipeWireRecordingApp>(app);
        if (!pipeWireApp)
        {
            return false;
        }

        bool success = false;
        auto nodes = this->nodes.copy();
        auto ports = this->ports.copy();

        for (const auto &[nodeId, node] : nodes)
        {
            if (node.name != app->name)
                continue;

            for (const auto &[portId, port] : ports)
            {
                if (port.direction == SPA_DIRECTION_OUTPUT && port.portAlias.find("soundux") != std::string::npos)
                {
                    for (const auto &[nodePortId, nodePort] : node.ports)
                    {
                        if (nodePort.direction == SPA_DIRECTION_INPUT)
                        {
                            if ((port.side == 'R' || port.side == '2') &&
                                (nodePort.side == 'R' || nodePort.side == '2'))
                            {
                                auto link = linkPorts(nodePortId, portId);

                                if (link)
                                {
                                    success = true;
                                    soundInputLinks.emplace_back(*link);
                                }
                            }
                            else if ((port.side == 'L' || port.side == '1') &&
                                     (nodePort.side == 'L' || nodePort.side == '1'))
                            {
                                auto link = linkPorts(nodePortId, portId);

                                if (link)
                                {
                                    success = true;
                                    soundInputLinks.emplace_back(*link);
                                }
                            }
                        }
                    }
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
        auto pipeWireApp = std::dynamic_pointer_cast<PipeWirePlaybackApp>(app);
        if (!pipeWireApp)
        {
            return false;
        }

        bool success = false;
        auto nodes = this->nodes.copy();
        auto ports = this->ports.copy();

        for (const auto &[nodeId, node] : nodes)
        {
            if (node.name != app->name)
                continue;

            for (const auto &[portId, port] : ports)
            {
                if (port.direction == SPA_DIRECTION_INPUT && port.portAlias.find("soundux") != std::string::npos)
                {
                    for (const auto &[nodePortId, nodePort] : node.ports)
                    {
                        if (nodePort.direction == SPA_DIRECTION_OUTPUT)
                        {
                            if ((port.side == 'R' || port.side == '2') &&
                                (nodePort.side == 'R' || nodePort.side == '2'))
                            {
                                auto link = linkPorts(portId, nodePortId);

                                if (link)
                                {
                                    success = true;
                                    passthroughLinks.emplace_back(*link);
                                }
                            }
                            else if ((port.side == 'L' || port.side == '1') &&
                                     (nodePort.side == 'L' || nodePort.side == '1'))
                            {
                                auto link = linkPorts(portId, nodePortId);

                                if (link)
                                {
                                    success = true;
                                    passthroughLinks.emplace_back(*link);
                                }
                            }
                        }
                    }
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