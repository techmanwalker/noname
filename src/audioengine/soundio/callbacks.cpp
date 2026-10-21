#include "ringbuffer/ringbuffer.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <soundio/soundio.h>

// Every helper below is internal to this TU and force-inlined: the compiler flattens
// them back into write_callback. No virtuals, no std::function, no pointers.
#define RT_INLINE [[gnu::always_inline]] inline

namespace {

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

RT_INLINE void
notify_tape_ran_out (audio_ring_buffer &ring)
{
    ring.rt_events.post(rt_event::tape_ran_out);
}

RT_INLINE void
notify_boundary_hit (audio_ring_buffer &ring)
{
    ring.rt_events.post(rt_event::boundary_hit);
}

// --- decoder handshake

struct transport_snapshot
{
    bool flush_requested; // decoder wants head/tail to itself
    bool seek_pending;    // a seek is in flight
};

// "The RT thread is inside a callback". Constructed first thing in write_callback,
// destroyed on every exit path. Other half of audio_ring_buffer's flush protocol:
// the decoder only touches head/tail once it has seen flush_request set AND rt_active clear.
struct rt_activity_scope
{
    audio_ring_buffer &ring;

    explicit rt_activity_scope (audio_ring_buffer &r) : ring(r)
    { ring.rt_active.store(true, std::memory_order_seq_cst); }

    ~rt_activity_scope ()
    { ring.rt_active.store(false, std::memory_order_seq_cst); }
};

// Call once per callback, after rt_activity_scope is up and BEFORE any pop().
RT_INLINE transport_snapshot
read_transport (audio_ring_buffer &ring)
{
    return {
        ring.flush_request.load(std::memory_order_seq_cst),
        ring.pending_seeks.load(std::memory_order_acquire) > 0
    };
}

// --- ring side

// Seam between "ring speaks floats" and "ring speaks frames of any format".
template <pcm_sample S>
RT_INLINE int
pop_frames (audio_ring_buffer &ring, S *dst, int frames)
{
    return static_cast<int>(ring.pop(reinterpret_cast<std::byte *>(dst), size_t(frames)));
}

// A short read after the decoder pushed the last of A's samples with no B behind them: the tape ran out.
// One-shot: eof_played.exchange guarantees only the first underrun announces it.
RT_INLINE void
handle_tape_ran_out (audio_ring_buffer &ring, bool underran)
{
    if (!underran || !ring.rt_claim_tape_end()) return;

    ring.is_paused.store(true, std::memory_order_release);
    notify_tape_ran_out(ring);
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

// Two counters, same increment: frames_played feeds playback_position_ms() (UI time),
// absolute_frames_played feeds gapless boundary detection. Returns the absolute value.
RT_INLINE uint64_t
count_played_frames (audio_ring_buffer &ring, int frames)
{
    const auto n = static_cast<uint64_t>(frames);
    ring.frames_played.fetch_add(n, std::memory_order_relaxed);
    return ring.absolute_frames_played.fetch_add(n, std::memory_order_relaxed) + n;
}

// Fires once when playback reaches the frame the decoder armed where A ends and B begins.
RT_INLINE void
check_boundary_hit (audio_ring_buffer &ring, uint64_t played)
{
    if (ring.rt_crossed_boundary(played)) notify_boundary_hit(ring);
}

// --- the loop

struct write_ctx
{
    SoundIoOutStream  *out;
    audio_ring_buffer &ring;
    transport_snapshot snap;
    float              gain;
    int                frames_max;
};

// is_paused is intentionally read per chunk (see notes), the rest comes from the snapshot.
RT_INLINE bool
output_is_muted (const write_ctx &c)
{
    return c.snap.flush_requested
        || c.snap.seek_pending
        || c.ring.is_paused.load(std::memory_order_acquire);
}

// false = error or no room; libsoundio's contract says: do NOT call end_write then.
// Deliberately NOT execute_soundio(): its failure path logs with qCWarning (allocates).
RT_INLINE bool
begin_chunk (const write_ctx &c, SoundIoChannelArea **areas, int &frames)
{
    if (const int err = soundio_outstream_begin_write(c.out, areas, &frames)) {
        c.ring.rt_events.post_error(rt_event::begin_write_failed, err);
        return false;
    }
    return frames > 0;
}

RT_INLINE void
end_chunk (const write_ctx &c)
{
    if (const int err = soundio_outstream_end_write(c.out))
        c.ring.rt_events.post_error(rt_event::end_write_failed, err);
}

template <sample_format F>
RT_INLINE void
write_loop (const write_ctx &c) requires playable_format<F>
{
    using S = sample_type_t<F>;

    constexpr size_t scratch_samples = scratch_bytes / sizeof(S);

    const int ch        = c.ring.format().channels;
    const int chunk_max = ch > 0 ? int(scratch_samples) / ch : 0;

    // ring and stream must agree (checked at stream open): never spin on the RT thread
    if (ch < 1 || c.out->layout.channel_count != ch || chunk_max < 1) return;

    S scratch[scratch_samples]; // intentionally uninitialised: only the `got` frames are ever read

    int frames_left = c.frames_max;
    while (frames_left > 0) {
        int frames = std::min(frames_left, chunk_max);
        SoundIoChannelArea *areas;

        if (!begin_chunk(c, &areas, frames)) break;

        if (output_is_muted(c)) {
            feed_silence<S>(areas, ch, 0, frames);
        } else {
            const int got = pop_frames(c.ring, scratch, frames);

            handle_tape_ran_out(c.ring, got < frames);

            write_to_device(scratch, ch, got, c.gain, areas);
            feed_silence<S>(areas, ch, got, frames); // pad the underrun tail

            check_boundary_hit(c.ring, count_played_frames(c.ring, frames));
        }

        end_chunk(c);
        frames_left -= frames;
    }
}

// --- format dispatch: one switch per callback, never per sample

// One switch per callback. The ring format IS the device format (validated at stream open).
RT_INLINE void
dispatch_format (const write_ctx &c)
{
    switch (c.ring.format().sample) {
    case f32ne: write_loop<f32ne>(c); break;
    case f64ne: write_loop<f64ne>(c); break;
    case s32ne: write_loop<s32ne>(c); break;
    case s16ne: write_loop<s16ne>(c); break;
    default: break; // refused at negotiation, not discovered here
    }
}

} // namespace

/**
    @brief Output callback for libsoundio.

    @details Executed at a very high priority.
    Must never block, nor allocate (no new nor std::vector), nor print to console.
*/
void
write_callback(struct SoundIoOutStream *outstream, [[maybe_unused]] int frame_count_min, int frame_count_max)
{
    auto &ring = *static_cast<audio_ring_buffer*>(outstream->userdata);

    rt_activity_scope in_callback(ring);                   // 1. announce we're inside
    const transport_snapshot snap = read_transport(ring);   // 2. then look at the decoder's request

    dispatch_format({ outstream, ring, snap,
                      ring.volume_multiplier.load(std::memory_order_relaxed), frame_count_max });
}


// Error handling

void
error_callback(struct SoundIoOutStream *outstream, int err)
{
    static_cast<audio_ring_buffer*>(outstream->userdata)->rt_events.post_error(rt_event::stream_error, err);
}

void
underflow_callback(struct SoundIoOutStream *outstream)
{
    static_cast<audio_ring_buffer*>(outstream->userdata)->rt_events.post(rt_event::underflow);
}