#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <vector>

#include "audiortevents.hpp"
#include "audiostreamformat.hpp"

// sndio
void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);

void error_callback(struct SoundIoOutStream *outstream, int err);

void underflow_callback (struct SoundIoOutStream *outstream);

class audio_ring_buffer
{
    // (friend removed: the decoder no longer touches ring internals)

    void begin_rt_quiesce();
    void end_rt_quiesce();

    // immutable for the ring's whole life: a format/rate change builds a new ring.
    // declaration order matters (initialiser list).
    const stream_format m_format;
    const size_t        m_frame_bytes;     // m_format.bytes_per_frame()
    const size_t        m_capacity_frames; // slots; one stays free, usable = capacity - 1

    // how long push_blocking() sleeps before re-checking for room (see audioringbuffer.cpp)
    std::chrono::microseconds nap_for (size_t missing_frames) const;

    // raw bytes: the ring is a transport, it never interprets samples.
    std::vector<std::byte> buffer;

    std::atomic<size_t> head{0}; // in FRAMES: where FFmpeg writes
    std::atomic<size_t> tail{0}; // in FRAMES: where libsoundio reads

    // synchronization for a blocking push, fundamental to avoid skim decoding
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic_bool m_cancelled{false};

public:

    using seconds = std::chrono::duration<double>;

    explicit audio_ring_buffer (stream_format fmt, seconds capacity = std::chrono::seconds(10));

    const stream_format &format () const noexcept { return m_format; }

    // called by the ffmpeg thread. `frames` are FRAMES (one sample per channel), interleaved,
    // already in format(). Blocking - guaranteed to succeed unless cancelled.
    bool push_blocking(const std::byte* data, size_t frames);

    // cancel an in-flight blocking push (call before seek/reset)
    void cancel_blocking_push();

    // re-arm after cancel
    void reset_cancel();

    // called by libsoundio write_callback. Raw frames, NO gain: volume belongs to the device side.
    size_t pop (std::byte* dest, size_t max_frames);

    bool is_full () const;
    size_t writable_size() const;

    // implemented here to read during write_callback
    std::atomic_bool is_paused {false};

    // track eof
    std::atomic_bool eof_decoded {false}; // set by decoder when it flushes the last frame
    std::atomic_bool eof_played {false};  // set by libsoundio when the buffer drains

    
    // Playback time tracking

    // last manually seeked/loaded timestamp, where frames_played = 0 corresponds to
    std::atomic_uint64_t playback_base_ms {0};

    // delay, in frames, after the playback_base_ms time, never reset by write_callback
    std::atomic_uint64_t frames_played {0}; 

    // Current playback position in ms, derived from the above.
    uint64_t playback_position_ms() const;

    // Stream data
    uint64_t get_sample_rate () const;

    // Tracks total frames pushed by the decoder since the engine started
    std::atomic_uint64_t absolute_frames_decoded{0};
    
    // Tracks total frames consumed by libsoundio
    std::atomic_uint64_t absolute_frames_played{0};

    // Tracks ongoing seek operations to mute the hardware loop
    std::atomic<int> pending_seeks{0};

    // Governed by a perceptual volume control that rescales logarithmic values back for soundio
    std::atomic<float> volume_multiplier{1.0f};
    
    // Flush protocol, decoder thread <-> RT callback (Dekker-style, all seq_cst):
    //   decoder: flush_request = true;  then waits while rt_active == true
    //   RT:      rt_active = true;      then reads flush_request
    // Whoever loses the race is guaranteed to see the other's store, so once
    // begin_rt_quiesce() returns, the RT thread cannot touch head/tail until
    // end_rt_quiesce(), and nobody ever waits on a callback that isn't running.
    std::atomic_bool flush_request{false};
    std::atomic_bool rt_active{false};

    // RAII: holds the RT thread out of head/tail for its lifetime (protocol above).
    class rt_quiesce
    {
    public:
        explicit rt_quiesce (audio_ring_buffer &r) : m_ring(r) { m_ring.begin_rt_quiesce(); }
        ~rt_quiesce () { m_ring.end_rt_quiesce(); }
        rt_quiesce (const rt_quiesce &) = delete;
        rt_quiesce &operator= (const rt_quiesce &) = delete;
    private:
        audio_ring_buffer &m_ring;
    };

    // Decoder thread only. Empties the ring and rewinds every clock/flag to "a track that
    // starts at base_ms". The ONE place that knows which ring state belongs to a playback
    // position: a new field gets reset here or nowhere. Deliberately NOT included:
    // m_cancelled and pending_seeks (control-thread handshakes, not playback state).
    void reset (const rt_quiesce &, uint64_t base_ms);

    std::mutex boundary_mutex;
    bool has_upcoming_boundary{false};
    uint64_t upcoming_boundary_frame{0};
    uint64_t clock_origin_frame{0};   // absolute frame where the current track began; guarded by boundary_mutex

    // Target frame count for the next upcoming track boundary (UINT64_MAX when none)
    std::atomic_uint64_t next_boundary_frame{UINT64_MAX};

    // RT thread -> Qt thread events (playlist finished, track boundary, stream error/underflow).
    // See audiortevents.hpp; the Qt side is rt_event_pump.
    rt_event_mailbox rt_events;

    bool check_for_boundary_and_advance();
};