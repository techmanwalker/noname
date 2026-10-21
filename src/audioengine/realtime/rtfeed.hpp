#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "realtime/rteventmailbox.hpp"   // rt_event
#include "ringbuffer/streamformats.hpp"  // stream_format

// One callback's worth of "whatever feeds the device". Everything here is RT: wait-free, no allocation.
template <typename P>
concept rt_pass = requires (P &p, std::byte *dst, std::size_t n, int frames, uint64_t played, rt_event e, int err)
{
    { p.muted () }                  noexcept -> std::same_as<bool>;          // flush/seek in flight, or paused
    { p.gain () }                   noexcept -> std::same_as<float>;
    { p.pop (dst, n) }              noexcept -> std::same_as<std::size_t>;   // frames actually delivered
    { p.count_played (frames) }     noexcept -> std::same_as<uint64_t>;      // absolute frames played so far
    { p.crossed_boundary (played) } noexcept -> std::same_as<bool>;          // true once per boundary
    { p.claim_tape_end () }         noexcept -> std::same_as<bool>;          // true once per tape end; pauses
    { p.notify (e) }                noexcept;
    { p.notify_error (e, err) }     noexcept;
};

template <typename F>
concept rt_feed = requires (F &f)
{
    { std::as_const(f).format () } noexcept -> std::convertible_to<const stream_format &>;
    { f.begin_pass () }            noexcept -> rt_pass;
};