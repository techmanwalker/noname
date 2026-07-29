#pragma once

#include "audioengine.hpp"

// Friendlier error messages, return the errcode itself
template<typename Func, typename... Args>
int 
audio_engine::log_soundio_internal(const char* func_name, Func soundio_func, Args&&... args) {
    int errcode = soundio_func(std::forward<Args>(args)...);

    if (errcode != 0) {
        qCWarning(l_soundio) 
            << func_name 
            << " failed with exit code " 
            << errcode 
            << ": " 
            << soundio_strerror(errcode);
    }

    return errcode; // nesting support
}