#pragma once
#if defined(__linux__)
#include <cstdint>
#include <pipewire/pipewire.h>

namespace Soundux
{
    namespace PipeWireApi
    {
        void setup();

        inline pw_core *(*pw_context_connect)(pw_context *, pw_properties *, std::size_t);
        inline pw_context *(*pw_context_new)(pw_loop *, pw_properties *, std::size_t);
        inline pw_main_loop *(*pw_main_loop_new)(pw_properties *);
        inline pw_loop *(*pw_main_loop_get_loop)(pw_main_loop *);

        inline void (*pw_proxy_add_listener)(pw_proxy *, spa_hook *, pw_proxy_events *, void *);
        inline int (*pw_properties_setf)(pw_properties *, const char *, const char *, ...);
        inline int (*pw_properties_set)(pw_properties *, const char *, const char *);
        inline pw_properties *(*pw_properties_new)(const char *, ...);
        inline void (*pw_main_loop_destroy)(pw_main_loop *);
        inline void (*pw_properties_free)(pw_properties *);
        inline int (*pw_main_loop_quit)(pw_main_loop *);
        inline void (*pw_context_destroy)(pw_context *);
        inline int (*pw_main_loop_run)(pw_main_loop *);
        inline int (*pw_core_disconnect)(pw_core *);
        inline void (*pw_proxy_destroy)(pw_proxy *);
        inline void (*pw_init)(int *, char **);

    } // namespace PipeWireApi
} // namespace Soundux
#endif