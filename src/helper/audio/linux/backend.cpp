#if defined(__linux__)
#include "backend.hpp"
#include "pipewire/pipewire.hpp"
#include "pulseaudio/pulseaudio.hpp"
#include <core/enums/enums.hpp>
#include <core/global/globals.hpp>
#include <fancy.hpp>
#include <memory>

namespace Soundux::Objects
{
    std::shared_ptr<AudioBackend> AudioBackend::createInstance(Enums::BackendType backend)
    {
        std::shared_ptr<AudioBackend> instance;
        if (backend == Enums::BackendType::PulseAudio)
        {
            instance = std::shared_ptr<PulseAudio>(new PulseAudio()); // NOLINT
            auto pulseInstance = std::dynamic_pointer_cast<PulseAudio>(instance);

            if (pulseInstance && pulseInstance->setup())
            {
                if (!pulseInstance->switchOnConnectPresent())
                {
                    if (pulseInstance->loadModules())
                    {
                        return instance;
                    }
                }
                else
                {
                    return instance;
                }
            }

            if (pulseInstance && pulseInstance->isRunningPipeWire())
            {
                backend = Enums::BackendType::PipeWire;
                Globals::gSettings.audioBackend = backend;
            }
        }

        if (backend == Enums::BackendType::PipeWire)
        {
            instance = std::shared_ptr<PipeWire>(new PipeWire()); // NOLINT
            if (instance->setup())
            {
                return instance;
            }
        }

        Fancy::fancy.logTime().failure() << "Failed to create AudioBackend instance" << std::endl;
        Globals::gSettings.audioBackend = Enums::BackendType::None;
        return nullptr;
    }
} // namespace Soundux::Objects
#endif