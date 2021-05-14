#if defined(__linux__)
#include "forward.hpp"
#include <dlfcn.h>
#include <stdexcept>

template <typename T> void loadFunc(void *so, T &function, const std::string &name)
{
    function = reinterpret_cast<T>(dlsym(so, name.c_str()));

    if (function == nullptr)
    {
        throw std::runtime_error("Failed to load function " + name);
    }
}

void Soundux::PipeWireApi::setup()
{
    auto *libpulse = dlopen("libpipewire.so", RTLD_LAZY);
    if (libpulse)
    {
#define load(name) loadFunc(libpulse, name, #name)
        load(pw_context_connect);
        load(pw_context_new);
        load(pw_main_loop_new);
        load(pw_main_loop_get_loop);
        load(pw_proxy_add_listener);
        load(pw_core_add_listener);
        load(pw_properties_setf);
        load(pw_properties_set);
        load(pw_properties_new);
        load(pw_properties_free);
        load(pw_main_loop_destroy);
        load(pw_main_loop_quit);
        load(pw_context_destroy);
        load(pw_core_disconnect);
        load(pw_main_loop_run);
        load(pw_proxy_destroy);
        load(pw_init);
    }
    else
    {
        throw std::runtime_error("Failed to load pipewire");
    }
}

#endif