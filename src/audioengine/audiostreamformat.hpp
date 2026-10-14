#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

// What the ring can CARRY. Carrying is free (bytes); playing needs a kernel in
// soundiowritecallback.cpp and a decoder path in audiodecode.cpp.
enum class sample_format : uint8_t {
    u8, s8,
    s16le, s16be,
    s24le, s24be,   // 24 valid bits in a 32-bit container, low-aligned (libsoundio's S24)
    s32le, s32be,
    f32le, f32be,
    f64le, f64be,
    mulaw, alaw,    // opaque 8-bit companded bytes, never treated as PCM
};

struct sample_traits
{
    uint8_t bytes;      // container size inside the ring
    uint8_t valid_bits;
    bool    is_float, is_signed, big_endian, companded;
};

constexpr sample_traits
traits_of (sample_format f) noexcept
{
    switch (f) {
    case sample_format::u8:    return { 1,  8, false, false, false, false };
    case sample_format::s8:    return { 1,  8, false, true,  false, false };
    case sample_format::s16le: return { 2, 16, false, true,  false, false };
    case sample_format::s16be: return { 2, 16, false, true,  true,  false };
    case sample_format::s24le: return { 4, 24, false, true,  false, false };
    case sample_format::s24be: return { 4, 24, false, true,  true,  false };
    case sample_format::s32le: return { 4, 32, false, true,  false, false };
    case sample_format::s32be: return { 4, 32, false, true,  true,  false };
    case sample_format::f32le: return { 4, 32, true,  true,  false, false };
    case sample_format::f32be: return { 4, 32, true,  true,  true,  false };
    case sample_format::f64le: return { 8, 64, true,  true,  false, false };
    case sample_format::f64be: return { 8, 64, true,  true,  true,  false };
    case sample_format::mulaw:
    case sample_format::alaw:  return { 1,  8, false, true,  false, true  };
    }
    return {};
}

inline constexpr bool host_is_little_endian = std::endian::native == std::endian::little;

// The formats that have a playback kernel today, in host byte order.
inline constexpr sample_format s16ne = host_is_little_endian ? sample_format::s16le : sample_format::s16be;
inline constexpr sample_format s32ne = host_is_little_endian ? sample_format::s32le : sample_format::s32be;
inline constexpr sample_format f32ne = host_is_little_endian ? sample_format::f32le : sample_format::f32be;
inline constexpr sample_format f64ne = host_is_little_endian ? sample_format::f64le : sample_format::f64be;

struct stream_format
{
    sample_format sample;
    uint32_t      rate;      // Hz
    uint8_t       channels;

    constexpr size_t bytes_per_frame () const noexcept { return traits_of(sample).bytes * channels; }
    friend constexpr bool operator== (const stream_format &, const stream_format &) = default;
};