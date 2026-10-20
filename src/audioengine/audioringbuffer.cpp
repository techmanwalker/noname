#include "audioringbuffer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <thread>

#include <QObject>

// The Ring buffer

namespace {
// +1: one slot stays free to tell full from empty, so the usable space is exactly `duration`
size_t
frames_for (uint32_t rate, audio_ring_buffer::seconds duration)
{
    return static_cast<size_t>(std::ceil(duration.count() * rate)) + 1;
}
}

audio_ring_buffer::audio_ring_buffer (stream_format fmt, seconds capacity)
    : m_format(fmt),
      m_frame_bytes(fmt.bytes_per_frame()),
      m_capacity_frames(frames_for(fmt.rate, capacity)),
      buffer(m_capacity_frames * m_frame_bytes)
{
    // rate == 0 or channels == 0 would divide by zero in nap_for()/playback_position_ms()
    Q_ASSERT(fmt.rate > 0 && fmt.channels > 0); // or std::assert / a thrown error, your call
}

bool
audio_ring_buffer::push_blocking(const std::byte* data, size_t count)
{
    // A request larger than the buffer can ever hold would deadlock
    if (count >= m_capacity_frames) return false;

    std::unique_lock<std::mutex> lock(m_mutex);
    
    // Nobody wakes us when the device frees space (the RT callback must not make syscalls),
    // so nap for as long as the device needs to drain the missing space, then re-check.
    // Every wait is bounded and the predicate is re-evaluated after it, so there is no
    // wakeup to lose. cancel_blocking_push() still wakes us instantly.
    while (!m_cancelled.load()) {
        const size_t room = writable_size();
        if (room >= count) break;
        m_cv.wait_for(lock, nap_for(count - room));
    }
    
    if (m_cancelled.load()) return false;
    
    // guaranteed to succeed - do the push (two memcpys instead of a per-sample modulo loop)
    const size_t h     = head.load(std::memory_order_relaxed);
    const size_t first = std::min(count, m_capacity_frames - h);
    std::memcpy(buffer.data() + h * m_frame_bytes, data, first * m_frame_bytes);
    if (count > first)
        std::memcpy(buffer.data(), data + first * m_frame_bytes, (count - first) * m_frame_bytes);
    head.store((h + count) % m_capacity_frames, std::memory_order_release);

    absolute_frames_decoded.fetch_add(count, std::memory_order_relaxed);

    return true;
}

size_t
audio_ring_buffer::pop(std::byte* dest, size_t max_frames)
{
    const size_t t = tail.load(std::memory_order_relaxed);
    const size_t h = head.load(std::memory_order_acquire);

    const size_t available = (h + m_capacity_frames - t) % m_capacity_frames;
    const size_t n         = std::min(max_frames, available);
    const size_t first     = std::min(n, m_capacity_frames - t);

    std::memcpy(dest, buffer.data() + t * m_frame_bytes, first * m_frame_bytes);
    if (n > first)
        std::memcpy(dest + first * m_frame_bytes, buffer.data(), (n - first) * m_frame_bytes);

    tail.store((t + n) % m_capacity_frames, std::memory_order_release);
    return n;
}

void
audio_ring_buffer::cancel_blocking_push()
{
    {
        // The mutex orders this store against the waiter's predicate check: no window
        // where the decoder checked, we notified, and only then it went to sleep.
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cancelled.store(true);
    }
    m_cv.notify_one();
}

void
audio_ring_buffer::reset_cancel()
{
    m_cancelled.store(false);
}

bool
audio_ring_buffer::is_full() const {
    return writable_size() == 0;
}

size_t
audio_ring_buffer::writable_size() const {
    size_t h = head.load(std::memory_order_acquire);
    size_t t = tail.load(std::memory_order_acquire);

    size_t used = (h + m_capacity_frames - t) % m_capacity_frames;
    return (m_capacity_frames - 1) - used;
}

// have we crossed yet?
bool
audio_ring_buffer::check_for_boundary_and_advance()
{
    std::lock_guard<std::mutex> lock(boundary_mutex);
    if (!has_upcoming_boundary
        || absolute_frames_played.load(std::memory_order_relaxed) < upcoming_boundary_frame)
        return false;

    has_upcoming_boundary = false;
    ++boundary_epoch;   // any seek issued before this moment is about a track that is gone

    // Rebase the UI clock to the new track. fetch_sub, not store(0): the RT thread keeps
    // adding to frames_played while we run, and a store would erase both its concurrent
    // increments and the new-track frames it already played since the crossing.
    frames_played.fetch_sub(upcoming_boundary_frame - clock_origin_frame, std::memory_order_relaxed);
    clock_origin_frame = upcoming_boundary_frame;
    playback_base_ms.store(0, std::memory_order_relaxed);
    return true;
}

uint64_t
audio_ring_buffer::playback_position_ms() const
{
    uint64_t frames  = frames_played.load(std::memory_order_relaxed);
    uint64_t base_ms = playback_base_ms.load(std::memory_order_relaxed);

    return base_ms + (frames * 1000) / m_format.rate;
}

uint64_t
audio_ring_buffer::get_sample_rate () const
{
    return m_format.rate;
}

std::chrono::microseconds
audio_ring_buffer::nap_for (size_t missing_frames) const
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    // Device not draining: the ring is full, so there are seconds of audio of slack.
    if (is_paused.load(std::memory_order_relaxed)) return 250ms;

    // Time the device needs to consume the missing space.
        const microseconds drain(static_cast<int64_t>(missing_frames * 1'000'000 / m_format.rate));

    return std::clamp<microseconds>(drain, 1ms, 50ms);
}

void
audio_ring_buffer::begin_rt_quiesce ()
{
    flush_request.store(true, std::memory_order_seq_cst);

    // Bounded by ONE callback's duration, and zero if no callback is running
    // (paused/stopped/dead stream): it can no longer wait for something that never comes.
    while (rt_active.load(std::memory_order_seq_cst)) std::this_thread::yield();
}

void
audio_ring_buffer::end_rt_quiesce ()
{
    flush_request.store(false, std::memory_order_release);
}

void
audio_ring_buffer::reset (const rt_quiesce &, uint64_t base_ms)
{
    // Nothing here is a synchronisation point, the quiesce is: begin_rt_quiesce() acquires
    // the RT thread's last pop (rt_active), end_rt_quiesce() releases these stores to its
    // next callback (flush_request). Relaxed is enough.
    head.store(0, std::memory_order_relaxed);
    tail.store(0, std::memory_order_relaxed);

    // frames first, base second: a UI read between the two sees the old base, never
    // new base + old frames
    frames_played.store(0, std::memory_order_relaxed);
    playback_base_ms.store(base_ms, std::memory_order_relaxed);

    absolute_frames_decoded.store(0, std::memory_order_relaxed);
    absolute_frames_played.store(0, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(boundary_mutex);
        has_upcoming_boundary = false;
        clock_origin_frame    = 0;   // see check_for_boundary_and_advance()
    }
    next_boundary_frame.store(UINT64_MAX, std::memory_order_relaxed);

    eof_decoded.store(false, std::memory_order_relaxed);
    eof_played.store(false, std::memory_order_relaxed);
}

audio_ring_buffer::boundary_view
audio_ring_buffer::boundary_snapshot () const
{
    std::lock_guard<std::mutex> lock(boundary_mutex);
    if (!has_upcoming_boundary) return { boundary_phase::none, boundary_epoch };

    const bool reached = absolute_frames_played.load(std::memory_order_relaxed) >= upcoming_boundary_frame;
    return { reached ? boundary_phase::reached : boundary_phase::armed, boundary_epoch };
}

bool
audio_ring_buffer::skip_within_armed_tail (const rt_quiesce &, uint64_t target_ms)
{
    const uint64_t now_ms = playback_position_ms();
    if (target_ms < now_ms) return false;

    uint64_t boundary;
    { std::lock_guard<std::mutex> lock(boundary_mutex); boundary = upcoming_boundary_frame; }

    const uint64_t played    = absolute_frames_played.load(std::memory_order_relaxed);
    const uint64_t remaining = boundary > played ? boundary - played : 0;   // A frames still in the ring
    const uint64_t skip      = std::min((target_ms - now_ms) * m_format.rate / 1000, remaining);

    // Same bookkeeping the RT thread would have done by playing those frames. Landing on
    // `remaining` makes the next unmuted callback cross the boundary normally.
    tail.store((tail.load(std::memory_order_relaxed) + skip) % m_capacity_frames, std::memory_order_relaxed);
    frames_played.fetch_add(skip, std::memory_order_relaxed);
    absolute_frames_played.fetch_add(skip, std::memory_order_relaxed);
    return true;
}