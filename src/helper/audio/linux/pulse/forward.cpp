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

void Soundux::PulseApi::setup()
{
    auto *libpulse = dlopen("libpulse.so", RTLD_LAZY);
    if (libpulse)
    {
#define load(name) loadFunc(libpulse, name, #name)
        load(pa_mainloop_new);
        load(pa_mainloop_iterate);
        load(pa_mainloop_get_api);
        load(pa_context_new);
        load(pa_context_connect);
        load(pa_context_set_state_callback);
        load(pa_context_load_module);
        load(pa_context_get_module_info_list);
        load(pa_context_get_source_output_info_list);
        load(pa_context_get_sink_input_info_list);
        load(pa_context_get_server_info);
        load(pa_proplist_gets);
        load(pa_context_set_default_source);
        load(pa_context_move_sink_input_by_name);
        load(pa_context_get_server_info);
        load(pa_context_move_sink_input_by_index);
        load(pa_context_move_source_output_by_name);
        load(pa_context_move_source_output_by_index);
        load(pa_context_set_sink_input_mute);
        load(pa_context_unload_module);
        load(pa_context_get_state);
        load(pa_operation_get_state);
    }
    else
    {
        throw std::runtime_error("Failed to load pulseaudio");
    }
}

#endif