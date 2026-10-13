#pragma once

#include "l_audioengine.hpp"
#include "audiointernalcontroller.hpp"

#include <soundio/soundio.h>

// Friendlier error messages, return the errcode itself.
// NOT real-time safe (qCWarning allocates and locks): never call this from write_callback,
// error_callback or underflow_callback. The RT path reports through audio_ring_buffer::rt_events.
template<typename Func, typename... Args>
int 
audio_internal_controller::log_soundio_internal(const char* func_name, Func soundio_func, Args&&... args) {
    int errcode = soundio_func(std::forward<Args>(args)...);

    if (errcode != 0) {
        qCWarning(l_soundio) 
            << func_name 
            << " failed with exit code " 
            << errcode 
            << ": " 
            << soundio_strerror(errcode);
    }

    return errcode;
}