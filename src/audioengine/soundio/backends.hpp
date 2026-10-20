#pragma once

#include <soundio/soundio.h>

#include <array>
#include <string>
#include <string_view>

// i'm begging for full support of c++26 reflection right now i swear

enum class audio_backend {
    pulseaudio,
    jack,
    alsa,
    wasapi,
    coreaudio,
    dummy,
    none
};

// iterable
inline constexpr std::array<audio_backend, 7> audio_backends {
    audio_backend::pulseaudio,
    audio_backend::jack,
    audio_backend::alsa,
    audio_backend::wasapi,
    audio_backend::coreaudio,
    audio_backend::dummy,
    audio_backend::none
};

namespace map
{

constexpr SoundIoBackend backend_to_soundio_backend (const audio_backend backend)
{
    using enum audio_backend;
    switch (backend) {
        case pulseaudio: return SoundIoBackendPulseAudio;
        case jack:       return SoundIoBackendJack;
        case alsa:       return SoundIoBackendAlsa;
        case wasapi:     return SoundIoBackendWasapi;
        case coreaudio:  return SoundIoBackendCoreAudio;
        case dummy:      return SoundIoBackendDummy;
        case none:       return SoundIoBackendNone;
        default:         return SoundIoBackendDummy;
    }
}

constexpr
std::string_view
backend_to_string_view(const audio_backend backend)
{
    using enum audio_backend;
    switch (backend) {
        case pulseaudio: return "pulseaudio";
        case jack:       return "jack";
        case alsa:       return "alsa";
        case wasapi:     return "wasapi";
        case coreaudio:  return "coreaudio";
        case dummy:      return "dummy";
        case none:       return "none";
        default:         return "unknown";
    }
}

// To generate the enunciate_backend string at compile time
namespace {

// calculate the exact character buffer length including separators
constexpr size_t
total_backends_string_size () {
    size_t total = 0;
    for (size_t i = 0; i < audio_backends.size(); ++i) {
        total += backend_to_string_view(audio_backends[i]).size();
        if (i < audio_backends.size() - 1) total += 2; // ", "
    }
    return total;
}

// generate a flat char array within a constexpr evaluation context
constexpr auto
generate_backends_string() {
    constexpr size_t N = total_backends_string_size();
    std::array<char, N + 1> result{}; // +1 for the null-terminator '\0'
    
    size_t index = 0;
    for (size_t i = 0; i < audio_backends.size(); ++i) {
        std::string_view sv = backend_to_string_view(audio_backends[i]);
        for (char c : sv) {
            result[index++] = c;
        }
        if (i < audio_backends.size() - 1) {
            result[index++] = ',';
            result[index++] = ' ';
        }
    }
    result[N] = '\0';
    return result;
}

// bake the characters permanently into the binary static memory
inline constexpr auto backends_char_buffer = generate_backends_string();

}

// expose the buffer as a safe, static global view
inline constexpr std::string_view audio_backends_list{
    backends_char_buffer.data(), 
    backends_char_buffer.size() - 1
};

constexpr
audio_backend
string_to_backend (const std::string &str)
{
    using enum audio_backend;

    if (str == "pulseaudio") return pulseaudio; else
    if (str == "jack")       return jack;       else
    if (str == "alsa")       return alsa;       else
    if (str == "wasapi")     return wasapi;     else
    if (str == "coreaudio")  return coreaudio;  else
    if (str == "dummy")      return dummy;      else
    if (str == "none")       return none;       else

    return dummy;
}

}