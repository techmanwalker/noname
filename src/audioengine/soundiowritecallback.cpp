#include "l_audioengine.hpp" // logging categories only
#include "audiointernalcontroller.hpp"
#include "audioringbuffer.hpp"

#include <soundio/soundio.h>

// Every helper below is internal to this TU and force-inlined: the compiler flattens
// them back into write_callback. No virtuals, no std::function, no pointers.
#define RT_INLINE [[gnu::always_inline]] inline

namespace {

constexpr size_t scratch_bytes = 32 * 1024; // stack scratch per callback (was 8192 floats)

// --- formats

// What the ring holds. Today: interleaved float32 stereo, hard-wired.
// ring_format_of() is the ONE place that changes when the ring becomes format-generic.
enum class sample_format : uint8_t { f32 };

struct ring_format
{
    sample_format format;
    int           channels;
};

RT_INLINE ring_format
ring_format_of ([[maybe_unused]] const audio_ring_buffer &ring)
{
    return { sample_format::f32, 2 };
}

// --- sample conversion (pivot: float)

template <typename T>
RT_INLINE float
to_float (T v)
{
    if constexpr (std::is_floating_point_v<T>) {
        return static_cast<float>(v);
    } else {
        constexpr double inv = 1.0 / (double(std::numeric_limits<T>::max()) + 1.0);
        return static_cast<float>(double(v) * inv);
    }
}

template <typename T>
RT_INLINE T
from_float (float v)
{
    if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(v);
    } else {
        constexpr double scale = double(std::numeric_limits<T>::max());
        return static_cast<T>(std::lrint(std::clamp(double(v), -1.0, 1.0) * scale));
    }
}

RT_INLINE char *
sample_slot (const SoundIoChannelArea &area, int frame)
{
    return area.ptr + static_cast<ptrdiff_t>(area.step) * frame;
}

template <typename Out>
RT_INLINE void
store_sample (char *slot, float v)
{
    *reinterpret_cast<Out *>(slot) = from_float<Out>(v);
}

// --- notifications (RT → rest of the app)
// The ONLY two places the RT thread talks to Qt. See the note below on why
// they are isolated.

RT_INLINE void
notify_playlist_finished (audio_ring_buffer &ring)
{
    if (ring.rt_notify_target) {
        emit ring.rt_notify_target->playlist_finished();
    }
}

RT_INLINE void
notify_track_boundary (audio_ring_buffer &ring)
{
    if (ring.rt_notify_target) {
        emit ring.rt_notify_target->track_boundary_crossed();
    }
}

// --- decoder handshake

struct transport_snapshot
{
    bool flush_requested; // decoder wants head/tail to itself
    bool seek_pending;    // a seek is in flight
};

// Run exactly once per callback, BEFORE any pop(). Publishing the ack at the start of
// a callback proves the previous callback has fully returned, so the decoder may reset
// head/tail while we keep feeding silence from the snapshot.
RT_INLINE transport_snapshot
handshake_with_decoder (audio_ring_buffer &ring)
{
    const bool flush = ring.flush_request.load(std::memory_order_acquire);
    const bool seek  = ring.pending_seeks.load(std::memory_order_acquire) > 0;

    ring.flush_ack.store(flush, std::memory_order_release);

    return { flush, seek };
}

// --- ring side

// Seam between "ring speaks floats" and "ring speaks frames of any format".
template <typename In>
RT_INLINE int
pop_frames (audio_ring_buffer &ring, In *dst, int frames, int channels)
{
    static_assert(std::is_same_v<In, float>,
                  "audio_ring_buffer::pop() only produces float today: extend this seam "
                  "when the ring becomes format-generic");
    // Invariant: the ring only ever holds whole frames (push sizes are multiples of channels).
    return static_cast<int>(ring.pop(dst, size_t(frames) * channels) / size_t(channels));
}

// A short read while the decoder has flushed its last frame is the true end of playback.
// One-shot: eof_played.exchange guarantees only the first underrun announces it.
RT_INLINE void
handle_end_of_playback (audio_ring_buffer &ring, bool underran)
{
    if (!underran || !ring.eof_decoded.load(std::memory_order_acquire)) return;
    if (ring.eof_played.exchange(true)) return;

    ring.is_paused.store(true, std::memory_order_release);
    notify_playlist_finished(ring);
}

// --- device side

// Silence on frames [first, last). Valid for every supported format (all signed/float: 0 == silence).
template <typename Out>
RT_INLINE void
feed_silence (const SoundIoChannelArea *areas, int out_ch, int first, int last)
{
    for (int f = first; f < last; ++f)
        for (int ch = 0; ch < out_ch; ++ch)
            store_sample<Out>(sample_slot(areas[ch], f), 0.0f);
}

// Channel policy, in one place (replace with a mixing matrix when >2 channels matter):
//   N -> 1 : equal-gain average (stereo: exactly 0.5 * (L + R), as before)
//   1 -> N : duplicate to every device channel
//   else   : channel i -> channel i; surplus device channels silent, surplus ring channels dropped
template <typename In, typename Out>
RT_INLINE void
write_to_device (const In *src, int in_ch, int frames, const SoundIoChannelArea *areas, int out_ch)
{
    if (out_ch == 1) {
        const float gain = 1.0f / float(in_ch);
        for (int f = 0; f < frames; ++f) {
            const In *s = src + size_t(f) * in_ch;
            float acc = 0.0f;
            for (int c = 0; c < in_ch; ++c) acc += to_float(s[c]);
            store_sample<Out>(sample_slot(areas[0], f), acc * gain);
        }
    } else if (in_ch == 1) {
        for (int f = 0; f < frames; ++f) {
            const float v = to_float(src[f]);
            for (int ch = 0; ch < out_ch; ++ch)
                store_sample<Out>(sample_slot(areas[ch], f), v);
        }
    } else {
        const int shared = std::min(in_ch, out_ch);
        for (int f = 0; f < frames; ++f) {
            const In *s = src + size_t(f) * in_ch;
            for (int ch = 0; ch < shared; ++ch)
                store_sample<Out>(sample_slot(areas[ch], f), to_float(s[ch]));
            for (int ch = shared; ch < out_ch; ++ch)
                store_sample<Out>(sample_slot(areas[ch], f), 0.0f);
        }
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

// Fires once when playback crosses the frame the decoder armed at the track boundary.
RT_INLINE void
check_track_boundary (audio_ring_buffer &ring, uint64_t played)
{
    const uint64_t target = ring.next_boundary_frame.load(std::memory_order_acquire);
    if (target == UINT64_MAX || played < target) return;

    ring.next_boundary_frame.store(UINT64_MAX, std::memory_order_release); // disarm first
    notify_track_boundary(ring);
}

// --- the loop

struct write_ctx
{
    SoundIoOutStream  *out;
    audio_ring_buffer &ring;
    transport_snapshot snap;
    ring_format        ring_fmt;
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
RT_INLINE bool
begin_chunk (SoundIoOutStream *out, SoundIoChannelArea **areas, int &frames)
{
    if (execute_soundio(soundio_outstream_begin_write, out, areas, &frames) != 0) return false;
    return frames > 0;
}

template <typename In, typename Out>
RT_INLINE void
write_loop (const write_ctx &c)
{
    constexpr size_t scratch_samples = scratch_bytes / sizeof(In);

    const int in_ch     = c.ring_fmt.channels;
    const int out_ch    = c.out->layout.channel_count;
    const int chunk_max = in_ch > 0 ? int(scratch_samples) / in_ch : 0;

    if (in_ch < 1 || out_ch < 1 || chunk_max < 1) return; // malformed format: never spin on the RT thread

    In scratch[scratch_samples]; // intentionally uninitialised: only the `got` frames are ever read

    int frames_left = c.frames_max;
    while (frames_left > 0) {
        int frames = std::min(frames_left, chunk_max);
        SoundIoChannelArea *areas;

        if (!begin_chunk(c.out, &areas, frames)) break;

        if (output_is_muted(c)) {
            feed_silence<Out>(areas, out_ch, 0, frames);
        } else {
            const int got = pop_frames(c.ring, scratch, frames, in_ch);

            handle_end_of_playback(c.ring, got < frames);

            write_to_device<In, Out>(scratch, in_ch, got, areas, out_ch);
            feed_silence<Out>(areas, out_ch, got, frames); // pad the underrun tail

            check_track_boundary(c.ring, count_played_frames(c.ring, frames));
        }

        execute_soundio(soundio_outstream_end_write, c.out);
        frames_left -= frames;
    }
}

// --- format dispatch: one switch per callback, never per sample

template <typename In>
RT_INLINE void
dispatch_device_format (const write_ctx &c)
{
    switch (c.out->format) {
    case SoundIoFormatFloat32NE: write_loop<In, float>(c);   break;
    case SoundIoFormatFloat64NE: write_loop<In, double>(c);  break;
    case SoundIoFormatS32NE:     write_loop<In, int32_t>(c); break;
    case SoundIoFormatS16NE:     write_loop<In, int16_t>(c); break;
    default: break; // must be rejected when the stream is opened, not discovered here
    }
}

RT_INLINE void
dispatch_ring_format (const write_ctx &c)
{
    switch (c.ring_fmt.format) {
    case sample_format::f32: dispatch_device_format<float>(c); break;
    // new ring format = one enum value + one case here (+ pop_frames)
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

    // Runs on every callback, even if the format below turns out to be unsupported,
    // otherwise seek() would spin forever waiting for the ack.
    const transport_snapshot snap = handshake_with_decoder(ring);

    dispatch_ring_format({ outstream, ring, snap, ring_format_of(ring), frame_count_max });
}


// Error handling

void
error_callback([[maybe_unused]] struct SoundIoOutStream *outstream, int err)
{
    qCWarning(l_soundio) << "SoundIo outstream error:" << soundio_strerror(err);
}

void
underflow_callback ([[maybe_unused]] struct SoundIoOutStream *outstream)
{
    qCWarning(l_soundio) << "SoundIo outstream underflow";
};