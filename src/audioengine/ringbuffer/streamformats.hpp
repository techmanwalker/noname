#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

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

static_assert(std::numeric_limits<float>::is_iec559 && std::numeric_limits<double>::is_iec559
              && sizeof(float) == 4 && sizeof(double) == 8,
              "the f32/f64 traits mean IEEE 754 binary32/binary64");

namespace detail {

// The native C++ type whose object representation is exactly one ring sample, derived ONLY
// from the traits. void when no such type exists:
//   - companded (opaque bytes, not PCM)
//   - foreign byte order (a bswap step is needed, a plain type would read garbage)
//   - padded container (s24 in 32 bits): a type exists, but its range is not the sample's
//     range, so using it is a decision the kernel author must make, not something to inherit
//   - a width the language has no type for
template <sample_traits T>
consteval auto
storage_type ()
{
    constexpr bool native_order = T.bytes == 1 || T.big_endian == !host_is_little_endian;
    constexpr bool padded       = T.valid_bits != T.bytes * 8;

    if constexpr (T.companded || padded || !native_order)
        return std::type_identity<void>{};
    else if constexpr (T.is_float && T.bytes == sizeof(float))
        return std::type_identity<float>{};
    else if constexpr (T.is_float && T.bytes == sizeof(double))
        return std::type_identity<double>{};
    else if constexpr (T.is_float)
        return std::type_identity<void>{};
    else if constexpr (T.bytes == 1)
        return std::type_identity<std::conditional_t<T.is_signed, int8_t,  uint8_t >>{};
    else if constexpr (T.bytes == 2)
        return std::type_identity<std::conditional_t<T.is_signed, int16_t, uint16_t>>{};
    else if constexpr (T.bytes == 4)
        return std::type_identity<std::conditional_t<T.is_signed, int32_t, uint32_t>>{};
    else if constexpr (T.bytes == 8)
        return std::type_identity<std::conditional_t<T.is_signed, int64_t, uint64_t>>{};
    else
        return std::type_identity<void>{};
}

} // namespace detail

// The C++ type holding ONE sample of format F in the ring, or void if the language has none.
template <sample_format F>
using sample_type_t = typename decltype(detail::storage_type<traits_of(F)>())::type;

template <sample_format F>
concept has_native_type = !std::is_void_v<sample_type_t<F>>;

// Regression tests for the derivation itself, so a change to traits_of() shows up here first.
static_assert(std::is_same_v<sample_type_t<f32ne>, float>);
static_assert(std::is_same_v<sample_type_t<f64ne>, double>);
static_assert(std::is_same_v<sample_type_t<s32ne>, int32_t>);
static_assert(std::is_same_v<sample_type_t<s16ne>, int16_t>);
static_assert(!has_native_type<sample_format::s24le> && !has_native_type<sample_format::mulaw>);

struct stream_format
{
    sample_format sample;
    uint32_t      rate;      // Hz
    uint8_t       channels;

    constexpr size_t bytes_per_frame () const noexcept { return traits_of(sample).bytes * channels; }
    friend constexpr bool operator== (const stream_format &, const stream_format &) = default;
};