#if defined(__linux__)
#include "forward.hpp"
#include <core/global/globals.hpp>
#include <dlfcn.h>
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

bool Soundux::PulseApi::setup()
{
#if defined(USE_FLATPAK)
#define load(name) name = reinterpret_cast<decltype(name)>(pa_##name);
    load(mainloop_new);
    load(mainloop_iterate);
    load(mainloop_get_api);
    load(context_new);
    load(context_connect);
    load(context_set_state_callback);
    load(context_load_module);
    load(context_get_module_info_list);
    load(context_get_source_output_info_list);
    load(context_get_sink_input_info_list);
    load(context_get_server_info);
    load(proplist_gets);
    load(context_set_default_source);
    load(context_move_sink_input_by_name);
    load(context_get_server_info);
    load(context_move_sink_input_by_index);
    load(context_move_source_output_by_name);
    load(context_move_source_output_by_index);
    load(context_set_sink_input_mute);
    load(context_unload_module);
    load(context_get_state);
    load(operation_get_state);
    return true;
#else
    auto *libpulse = dlopen("libpulse.so.0", RTLD_LAZY);
    if (libpulse)
    {
        try
        {

#define stringify(what) #what
#define load(name) loadFunc(libpulse, name, stringify(pa_##name))
            load(mainloop_new);
            load(mainloop_iterate);
            load(mainloop_get_api);
            load(context_new);
            load(context_connect);
            load(context_set_state_callback);
            load(context_load_module);
            load(context_get_module_info_list);
            load(context_get_source_output_info_list);
            load(context_get_sink_input_info_list);
            load(context_get_server_info);
            load(proplist_gets);
            load(context_set_default_source);
            load(context_move_sink_input_by_name);
            load(context_get_server_info);
            load(context_move_sink_input_by_index);
            load(context_move_source_output_by_name);
            load(context_move_source_output_by_index);
            load(context_set_sink_input_mute);
            load(context_unload_module);
            load(context_get_state);
            load(operation_get_state);
            return true;
        }
        catch (std::exception &e)
        {
            Fancy::fancy.logTime().failure() << "Loading Functions failed: " << e.what() << std::endl;
        }
    }

    Fancy::fancy.logTime().failure() << "Failed to load pulseaudio" << std::endl;
    return false;
#endif
}

#endif