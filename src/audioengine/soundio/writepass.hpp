#pragma once

#include "realtime/rtfeed.hpp"

#include <algorithm>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <soundio/soundio.h>

// Every helper below is force-inlined: the compiler flattens them into the callback that
// instantiates them. No virtuals, no std::function. The loop is written against rt_feed /
// rt_pass and never names the ring: the real ring end, a test double or a stdin reader all
// plug in the same way.
#define RT_INLINE [[gnu::always_inline]] inline

namespace rt_cb {

// What every RT kernel in this TU silently relies on.
template <typename S>
concept pcm_sample =
       std::is_trivially_copyable_v<S>
    && (   (std::floating_point<S> && sizeof(S) <= 8)    // float, double
        || (std::signed_integral<S> && sizeof(S) <= 4))  // exact through apply_gain's double
    ;                                                    // unsigned (u8) is excluded on purpose: silence is 0x80

// A format this TU can play: the traits yield a native type (audiostreamformat.hpp)
// AND the kernels can operate on that type.
template <sample_format F>
concept playable_format = has_native_type<F> && pcm_sample<sample_type_t<F>>;

constexpr size_t scratch_bytes = 32 * 1024; // stack scratch per callback (was 8192 floats)

// --- sample conversion (pivot: float)

template <pcm_sample S>
RT_INLINE S &
sample_at (const SoundIoChannelArea &area, int frame)
{
    return *reinterpret_cast<S *>(area.ptr + static_cast<ptrdiff_t>(area.step) * frame);
}

template <pcm_sample S>
RT_INLINE S
apply_gain (S v, float gain)
{
    if constexpr (std::is_floating_point_v<S>) {
        return static_cast<S>(v * gain);
    } else {
        // double on purpose: an s32 does not fit float's 24-bit mantissa
        constexpr double lo = double(std::numeric_limits<S>::min());
        constexpr double hi = double(std::numeric_limits<S>::max());
        return static_cast<S>(std::lrint(std::clamp(double(v) * double(gain), lo, hi)));
    }
}

// --- notifications (RT → rest of the app)
// The ONLY two places the RT thread reports events. They only post to the mailbox
// (audiortevents.hpp); signals and logs are emitted by rt_event_pump on the Qt thread.

template <rt_pass P>
RT_INLINE void
notify_tape_ran_out (P &pass)
{
    pass.notify(rt_event::tape_ran_out);
}

template <rt_pass P>
RT_INLINE void
notify_boundary_hit (P &pass)
{
    pass.notify(rt_event::boundary_hit);
}

// --- decoder handshake
// The RT half of the flush protocol, the seek mute and the gain snapshot are one object: the
// rt_pass the feed creates per callback. Its constructor is "the RT thread is inside a callback".

// --- ring side

// Seam between "ring speaks floats" and "ring speaks frames of any format".
template <pcm_sample S, rt_pass P>
RT_INLINE int
pop_frames (P &pass, S *dst, int frames)
{
    return static_cast<int>(pass.pop(reinterpret_cast<std::byte *>(dst), size_t(frames)));
}

// A short read after the decoder pushed the last of A's samples with no B behind them: the tape ran out.
// One-shot: eof_played.exchange guarantees only the first underrun announces it.
template <rt_pass P>
RT_INLINE void
handle_tape_ran_out (P &pass, bool underran)
{
    if (!underran || !pass.claim_tape_end()) return;   // one-shot, and it pauses the transport

    notify_tape_ran_out(pass);
}

// --- device side

template <pcm_sample S>
RT_INLINE void
feed_silence (const SoundIoChannelArea *areas, int ch, int first, int last)
{
    for (int f = first; f < last; ++f)
        for (int c = 0; c < ch; ++c)
            sample_at<S>(areas[c], f) = S{}; // signed/float: 0 is silence (u8 will need 0x80)
}

// Ring channels == device channels (validated at stream open). Mono/surround
// adaptation is the decoder's swresample job, not an RT concern.
template <pcm_sample S>
RT_INLINE void
write_to_device (const S *src, int ch, int frames, float gain, const SoundIoChannelArea *areas)
{
    const bool unity = gain == 1.0f; // plain copy: this is the bit-perfect path
    for (int f = 0; f < frames; ++f)
        for (int c = 0; c < ch; ++c) {
            const S v = src[size_t(f) * ch + c];
            sample_at<S>(areas[c], f) = unity ? v : apply_gain(v, gain);
        }
}

// --- clocks

// Fires once when playback reaches the frame the decoder armed where A ends and B begins.
template <rt_pass P>
RT_INLINE void
check_boundary_hit (P &pass, uint64_t played)
{
    if (pass.crossed_boundary(played)) notify_boundary_hit(pass);
}

// --- the loop

template <rt_pass P>
struct write_ctx
{
    SoundIoOutStream    *out;
    P                   &pass;
    const stream_format &fmt;
    int                  frames_max;
};

// false = error or no room; libsoundio's contract says: do NOT call end_write then.
// Deliberately NOT execute_soundio(): its failure path logs with qCWarning (allocates).
template <rt_pass P>
RT_INLINE bool
begin_chunk (const write_ctx<P> &c, SoundIoChannelArea **areas, int &frames)
{
    if (const int err = soundio_outstream_begin_write(c.out, areas, &frames)) {
        c.pass.notify_error(rt_event::begin_write_failed, err);
        return false;
    }
    return frames > 0;
}

template <rt_pass P>
RT_INLINE void
end_chunk (const write_ctx<P> &c)
{
    if (const int err = soundio_outstream_end_write(c.out))
        c.pass.notify_error(rt_event::end_write_failed, err);
}

template <sample_format F, rt_pass P>
RT_INLINE void
write_loop (const write_ctx<P> &c) requires playable_format<F>
{
    using S = sample_type_t<F>;

    constexpr size_t scratch_samples = scratch_bytes / sizeof(S);

    const int ch        = c.fmt.channels;
    const int chunk_max = ch > 0 ? int(scratch_samples) / ch : 0;

    // ring and stream must agree (checked at stream open): never spin on the RT thread
    if (ch < 1 || c.out->layout.channel_count != ch || chunk_max < 1) return;

    S scratch[scratch_samples]; // intentionally uninitialised: only the `got` frames are ever read

    int frames_left = c.frames_max;
    while (frames_left > 0) {
        int frames = std::min(frames_left, chunk_max);
        SoundIoChannelArea *areas;

        if (!begin_chunk(c, &areas, frames)) break;

        if (c.pass.muted()) {
            feed_silence<S>(areas, ch, 0, frames);
        } else {
            const int got = pop_frames(c.pass, scratch, frames);

            handle_tape_ran_out(c.pass, got < frames);

            write_to_device(scratch, ch, got, c.pass.gain(), areas);
            feed_silence<S>(areas, ch, got, frames); // pad the underrun tail

            check_boundary_hit(c.pass, c.pass.count_played(frames));
        }

        end_chunk(c);
        frames_left -= frames;
    }
}

// --- format dispatch: one switch per callback, never per sample

// One switch per callback. The ring format IS the device format (validated at stream open).
template <rt_pass P>
RT_INLINE void
dispatch_format (const write_ctx<P> &c)
{
    switch (c.fmt.sample) {
    case f32ne: write_loop<f32ne>(c); break;
    case f64ne: write_loop<f64ne>(c); break;
    case s32ne: write_loop<s32ne>(c); break;
    case s16ne: write_loop<s16ne>(c); break;
    default: break; // refused at negotiation, not discovered here
    }
}

// The whole write callback over any feed. The libsoundio entry points (soundio/callbacks.cpp)
// are this, instantiated for the ring's rt_end.
template <rt_feed F>
RT_INLINE void
run_write_callback (F &feed, SoundIoOutStream *out, int frames_max)
{
    auto pass = feed.begin_pass();

    dispatch_format(write_ctx<decltype(pass)>{ out, pass, feed.format(), frames_max });
}

} // namespace rt_cb