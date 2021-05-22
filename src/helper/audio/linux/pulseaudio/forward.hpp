#pragma once
#if defined(__linux__)
#include <cstdint>
#include <pulse/pulseaudio.h>
#include <type_traits>

namespace Soundux
{
    namespace PulseApi
    {
        bool setup();

        //* We declare function pointers here so that we can use dlsym to assign them later.
#define pulse_forward_decl(function) inline std::add_pointer_t<decltype(pa_##function)> function;

        pulse_forward_decl(context_new);
        pulse_forward_decl(mainloop_new);
        pulse_forward_decl(proplist_gets);
        pulse_forward_decl(context_connect);
        pulse_forward_decl(mainloop_iterate);
        pulse_forward_decl(mainloop_get_api);
        pulse_forward_decl(context_get_state);
        pulse_forward_decl(operation_get_state);
        pulse_forward_decl(context_load_module);
        pulse_forward_decl(context_unload_module);
        pulse_forward_decl(context_get_server_info);
        pulse_forward_decl(context_set_state_callback);
        pulse_forward_decl(context_set_default_source);
        pulse_forward_decl(context_set_sink_input_mute);
        pulse_forward_decl(context_get_module_info_list);
        pulse_forward_decl(context_move_sink_input_by_name);
        pulse_forward_decl(context_move_sink_input_by_index);
        pulse_forward_decl(context_get_sink_input_info_list);
        pulse_forward_decl(context_move_source_output_by_name);
        pulse_forward_decl(context_get_source_output_info_list);
        pulse_forward_decl(context_move_source_output_by_index);
    } // namespace PulseApi
} // namespace Soundux
#endif