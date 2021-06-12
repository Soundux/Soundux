#if defined(__linux__)
#include "forward.hpp"
#include <core/global/globals.hpp>
#include <dlfcn.h>
#include <exception>
#include <fancy.hpp>
#include <stdexcept>

template <typename T> void loadFunc(void *so, T &function, const std::string &name)
{
    function = reinterpret_cast<T>(dlsym(so, name.c_str()));

    if (function == nullptr)
    {
        throw std::runtime_error("Failed to load function " + name);
    }
}

bool Soundux::PipeWireApi::setup()
{
    auto *libpulse = dlopen("libpipewire-0.3.so.0", RTLD_LAZY);
    if (libpulse)
    {
        try
        {
#define stringify(what) #what
#define load(name) loadFunc(libpulse, name, stringify(pw_##name))
            load(init);
            load(context_new);
            load(main_loop_run);
            load(main_loop_new);
            load(proxy_destroy);
            load(main_loop_quit);
            load(properties_new);
            load(properties_set);
            load(context_connect);
            load(properties_setf);
            load(context_destroy);
            load(properties_free);
            load(core_disconnect);
            load(main_loop_destroy);
            load(main_loop_get_loop);
            load(proxy_add_listener);
            return true;
        }
        catch (std::exception &e)
        {
            Fancy::fancy.logTime().failure() << "Loading Functions failed: " << e.what() << std::endl;
        }
    }

    Fancy::fancy.logTime().failure() << "Failed to load pipewire" << std::endl;
    return false;
}

#endif