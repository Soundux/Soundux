#pragma once
#if defined(__linux__)
#include <cstdint>
#include <pipewire/pipewire.h>

namespace Soundux
{
    namespace PipeWireApi
    {
        bool setup();

        //* We declare function pointers here so that we can use dlsym to assign them later.
        inline pw_core *(*context_connect)(pw_context *, pw_properties *, std::size_t);
        inline pw_context *(*context_new)(pw_loop *, pw_properties *, std::size_t);
        inline pw_main_loop *(*main_loop_new)(pw_properties *);
        inline pw_loop *(*main_loop_get_loop)(pw_main_loop *);

        inline void (*proxy_add_listener)(pw_proxy *, spa_hook *, pw_proxy_events *, void *);
        inline int (*properties_setf)(pw_properties *, const char *, const char *, ...);
        inline int (*properties_set)(pw_properties *, const char *, const char *);
        inline pw_properties *(*properties_new)(const char *, ...);
        inline void (*main_loop_destroy)(pw_main_loop *);
        inline void (*properties_free)(pw_properties *);
        inline int (*main_loop_quit)(pw_main_loop *);
        inline void (*context_destroy)(pw_context *);
        inline int (*main_loop_run)(pw_main_loop *);
        inline int (*core_disconnect)(pw_core *);
        inline void (*proxy_destroy)(pw_proxy *);
        inline void (*init)(int *, char **);
    } // namespace PipeWireApi
} // namespace Soundux
#endif