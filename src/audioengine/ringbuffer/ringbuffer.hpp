#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <vector>

#include "realtime/rteventmailbox.hpp"
#include "ringbuffer/streamformats.hpp"

/**
    The transport between the decoder and the device. Three roles touch it, on three threads, and
    each one reaches only what its job needs:

      control   (Qt thread)  the public methods below. Only the object that OWNS the ring can call
                             them (audio_internal_controller): transport state, clocks, boundaries.
      producer  (decoder)    make_producer(): samples in, boundaries armed, the flush quiesce.
      rt_end    (RT thread)  make_rt_end(): samples out, clocks, events. Minted by the libsoundio
                             callback entry (soundio/callbacks.cpp), nowhere else.

    Every atomic is private: a role can do what its handle spells out and nothing else.
*/
class audio_ring_buffer
{
public:
    using seconds = std::chrono::duration<double>;

    class producer;   // defined at the end of the class
    class rt_end;     // defined at the end of the class

    explicit audio_ring_buffer (stream_format fmt, seconds capacity = std::chrono::seconds(10));
    audio_ring_buffer (const audio_ring_buffer &) = delete;
    audio_ring_buffer &operator= (const audio_ring_buffer &) = delete;

    const stream_format &format () const noexcept { return m_format; }

    [[nodiscard]] producer make_producer () noexcept;
    [[nodiscard]] rt_end   make_rt_end   () noexcept;

    // none:  no B behind A (or its hit was already consumed)
    // armed: the decoder has opened B's file, A's samples are still being played
    // hit:   the RT thread went past the boundary (B is audible), consume_boundary_hit() has not run yet
    enum class boundary_phase { none, armed, hit };

    struct boundary_view { boundary_phase phase; uint64_t epoch; };

    // RAII: holds the RT thread out of head/tail for its lifetime (flush protocol below).
    // Only producer::quiesce() can make one.
    class rt_quiesce
    {
    public:
        ~rt_quiesce () { m_ring.end_rt_quiesce(); }
        rt_quiesce (const rt_quiesce &) = delete;
        rt_quiesce &operator= (const rt_quiesce &) = delete;
    private:
        friend class audio_ring_buffer::producer;
        explicit rt_quiesce (audio_ring_buffer &r) : m_ring(r) { m_ring.begin_rt_quiesce(); }
        audio_ring_buffer &m_ring;
    };

    // ---- control role (Qt thread) ----
    void set_paused (bool paused) noexcept { is_paused.store(paused); }
    bool paused () const noexcept          { return is_paused.load(); }
    void set_volume (float gain) noexcept;   // linear multiplier, finite and >= 0
    void begin_seek () noexcept;             // mutes the device until the decoder calls producer::end_seek()
    void cancel_blocking_push();             // cancel an in-flight blocking push (call before seek/reset)
    void reset_cancel();                     // re-arm after cancel
    uint64_t playback_position_ms() const;   // derived from the clocks
    uint64_t get_sample_rate () const;

    // One lock: phase and epoch are consistent with each other (no ceremony can slip between them)
    boundary_view boundary_snapshot () const;

    // true once per boundary, after the RT thread hit it: B becomes A in the clocks
    // (position counts from B's start) and boundary_epoch moves on.
    bool consume_boundary_hit();

    // The Qt end of the RT -> Qt channel: rt_event_pump drains it
    rt_event_mailbox &events () noexcept { return rt_events; }

private:
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

    // track eof
    std::atomic_bool eof_decoded {false}; // set by decoder when it flushes the last frame
    std::atomic_bool eof_played {false};  // set by libsoundio when the buffer drains

    // boundaries
    mutable std::mutex boundary_mutex;
    bool has_upcoming_boundary{false};
    uint64_t upcoming_boundary_frame{0};
    uint64_t clock_origin_frame{0};   // absolute frame where the current track began; guarded by boundary_mutex
    uint64_t boundary_epoch{0};       // bumped (under boundary_mutex) each time a hit is consumed: a request
                                      // issued under an older epoch was about an A that is gone
    
    // The boundary of upcoming_boundary_frame as the RT thread's one-shot trigger: an atomic mirror,
    // because the callback cannot take boundary_mutex. Armed, it equals upcoming_boundary_frame.
    // Lifetime differs: that one lives until consume_boundary_hit() (Qt-side bookkeeping), this one
    // only until the RT thread crosses it (UINT64_MAX = nothing to fire). Its disarm can lag the
    // consume by one chunk; the queued boundary_hit then finds nothing to consume, by design.
    // Writers: arm_boundary(), reset(), rt_crossed_boundary() only.
    std::atomic_uint64_t rt_trigger_frame{UINT64_MAX};

    // Decoder thread (through producer). `frames` are FRAMES (one sample per channel), interleaved,
    // already in format(). Blocking - guaranteed to succeed unless cancelled.
    bool push_blocking(const std::byte* data, size_t frames);

    // RT thread (through rt_end::pass). Raw frames, NO gain: volume belongs to the device side.
    size_t pop (std::byte* dest, size_t max_frames);

    bool is_full () const;
    size_t writable_size() const;

    // Transport state. Written by set_paused() (control) and pass::claim_tape_end() (RT),
    // read per chunk by the RT thread.
    std::atomic_bool is_paused {false};
    
    // Playback time tracking

    // last manually seeked/loaded timestamp, where frames_played = 0 corresponds to
    std::atomic_uint64_t playback_base_ms {0};

    // delay, in frames, after the playback_base_ms time, never reset by write_callback
    std::atomic_uint64_t frames_played {0}; 

    // Tracks total frames pushed by the decoder since the engine started
    std::atomic_uint64_t absolute_frames_decoded{0};
    
    // Tracks total frames consumed by libsoundio
    std::atomic_uint64_t absolute_frames_played{0};

    // Seeks in flight: begin_seek() (control) counts up, end_seek() (decoder) down. Device muted while > 0.
    std::atomic<int> pending_seeks{0};
    void end_seek () noexcept;

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

    // Decoder thread only. Empties the ring and rewinds every clock/flag to "a track that
    // starts at base_ms". The ONE place that knows which ring state belongs to a playback
    // position: a new field gets reset here or nowhere. Deliberately NOT included:
    // m_cancelled and pending_seeks (control-thread handshakes, not playback state).
    void reset (const rt_quiesce &, uint64_t base_ms);

    // Decoder thread, boundary armed: fast-forward through A's tail that is already in the ring.
    // false = target is behind the playhead (the ring only holds the future).
    bool skip_within_armed_tail (const rt_quiesce &, uint64_t target_ms);

    // Decoder thread, NOT under quiesce (runs concurrently with the RT thread by design):
    // A's last frame is the one just pushed. Publishes the boundary to the UI side and the RT side.
    void arm_boundary ();

    // Decoder thread: A's samples are all in the ring and no B follows.
    void mark_tape_ended ();

    // true after mark_tape_ended(), until the next reset(). Any thread (one atomic load).
    bool tape_ended () const noexcept { return eof_decoded.load(std::memory_order_acquire); }

    // RT thread, wait-free. Defined here so they flatten into write_callback (another TU).

    // true exactly once per armed boundary: when `played` has reached it.
    // Disarms before returning, so the caller only has to notify.
    bool rt_crossed_boundary (uint64_t played) noexcept
    {
        const uint64_t target = rt_trigger_frame.load(std::memory_order_acquire);
        if (target == UINT64_MAX || played < target) return false;

        rt_trigger_frame.store(UINT64_MAX, std::memory_order_release); // disarm first
        return true;
    }

    // true exactly once per tape end: the decoder flushed the last frame with no B behind it,
    // and the device has now drained it. One-shot through eof_played.
    bool rt_claim_tape_end () noexcept { return tape_ended() && !eof_played.exchange(true); }

    // RT thread -> Qt thread events (tape ran out, boundary hit, stream error/underflow).
    // See rteventmailbox.hpp; the Qt side is rt_event_pump, through events().
    rt_event_mailbox rt_events;

public:
    // ---- decoder thread ----
    class producer
    {
    public:
        const stream_format &format () const noexcept { return m_r.m_format; }

        bool push_blocking (const std::byte *data, size_t frames) { return m_r.push_blocking(data, frames); }
        bool is_full () const { return m_r.is_full(); }
        void reset_cancel () { m_r.reset_cancel(); }

        // Flush protocol, decoder half: holds the RT thread out of head/tail while it lives.
        // The ONLY way to get an rt_quiesce, and reset()/skip_within_armed_tail() demand one.
        [[nodiscard]] rt_quiesce quiesce () { return rt_quiesce(m_r); }
        void reset (const rt_quiesce &q, uint64_t base_ms) { m_r.reset(q, base_ms); }
        bool skip_within_armed_tail (const rt_quiesce &q, uint64_t target_ms)
        { return m_r.skip_within_armed_tail(q, target_ms); }

        boundary_view boundary_snapshot () const { return m_r.boundary_snapshot(); }
        void arm_boundary () { m_r.arm_boundary(); }
        void mark_tape_ended () { m_r.mark_tape_ended(); }
        bool tape_ended () const noexcept { return m_r.tape_ended(); }

        // Completes a seek that control began with begin_seek(): the device unmutes.
        void end_seek () noexcept { m_r.end_seek(); }

    private:
        friend class audio_ring_buffer;
        explicit producer (audio_ring_buffer &r) noexcept : m_r(r) {}
        audio_ring_buffer &m_r;
    };

    // ---- RT thread ----
    class rt_end
    {
    public:
        // One callback's access. Constructing it IS "the RT thread is inside a callback" (RT half of
        // the flush protocol), destroying it ends that on every exit path. Wait-free, allocation-free.
        class pass
        {
        public:
            ~pass () { m_r.rt_active.store(false, std::memory_order_seq_cst); }
            pass (const pass &) = delete;
            pass &operator= (const pass &) = delete;

            // Per chunk: a flush or seek was in flight when the pass began, or the transport is paused now.
            bool muted () const noexcept
            { return m_flush_requested || m_seek_pending || m_r.is_paused.load(std::memory_order_acquire); }

            float gain () const noexcept { return m_gain; }

            size_t pop (std::byte *dst, size_t frames) noexcept { return m_r.pop(dst, frames); }

            // Absolute frames played so far, including these.
            uint64_t count_played (int frames) noexcept
            {
                const auto n = static_cast<uint64_t>(frames);
                m_r.frames_played.fetch_add(n, std::memory_order_relaxed);
                return m_r.absolute_frames_played.fetch_add(n, std::memory_order_relaxed) + n;
            }

            bool crossed_boundary (uint64_t played) noexcept { return m_r.rt_crossed_boundary(played); }

            // One-shot per tape end; the transport pauses with it.
            bool claim_tape_end () noexcept
            {
                if (!m_r.rt_claim_tape_end()) return false;
                m_r.is_paused.store(true, std::memory_order_release);
                return true;
            }

            void notify (rt_event e) noexcept { m_r.rt_events.post(e); }
            void notify_error (rt_event e, int err) noexcept { m_r.rt_events.post_error(e, err); }

        private:
            friend class rt_end;
            explicit pass (audio_ring_buffer &r) noexcept : m_r(r)
            {
                m_r.rt_active.store(true, std::memory_order_seq_cst);                   // 1. announce we're inside
                m_flush_requested = m_r.flush_request.load(std::memory_order_seq_cst);  // 2. then look at the decoder's request
                m_seek_pending    = m_r.pending_seeks.load(std::memory_order_acquire) > 0;
                m_gain            = m_r.volume_multiplier.load(std::memory_order_relaxed);
            }

            audio_ring_buffer &m_r;
            bool  m_flush_requested = false;
            bool  m_seek_pending    = false;
            float m_gain            = 1.0f;
        };

        const stream_format &format () const noexcept { return m_r.m_format; }
        [[nodiscard]] pass begin_pass () noexcept { return pass(m_r); }

        // Outside a pass: libsoundio's error and underflow callbacks.
        void notify (rt_event e) noexcept { m_r.rt_events.post(e); }
        void notify_error (rt_event e, int err) noexcept { m_r.rt_events.post_error(e, err); }

    private:
        friend class audio_ring_buffer;
        explicit rt_end (audio_ring_buffer &r) noexcept : m_r(r) {}
        audio_ring_buffer &m_r;
    };
};

inline audio_ring_buffer::producer audio_ring_buffer::make_producer () noexcept { return producer(*this); }
inline audio_ring_buffer::rt_end   audio_ring_buffer::make_rt_end   () noexcept { return rt_end(*this); }