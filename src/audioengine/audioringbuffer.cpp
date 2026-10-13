#include "audioringbuffer.hpp"

#include <algorithm>
#include <thread>

// The Ring buffer

audio_ring_buffer::audio_ring_buffer (std::optional<size_t> requested_sample_rate)
    :  sample_rate (requested_sample_rate.value_or(48000)),
       buffer(capacity)
    
{
}

bool
audio_ring_buffer::__push_unlocked (const float* data, size_t count)
{
    // what do h and next_h contain here?
    size_t h = head.load(std::memory_order_relaxed);
    size_t next_h = (h + count) % capacity;
    
    // verify if there is enough space
    if ((next_h + capacity - tail.load(std::memory_order_acquire)) % capacity < count)
        return false; // full buffer

    // copy data
    for(size_t i = 0; i < count; ++i) buffer[(h + i) % capacity] = data[i];
    
    head.store(next_h, std::memory_order_release);
    return true;
}

bool
audio_ring_buffer::push_blocking(const float* data, size_t count)
{
    // A request larger than the buffer can ever hold would deadlock
    if (count >= capacity) return false;

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
    
    // guaranteed to succeed - do the push
    size_t h = head.load(std::memory_order_relaxed);
    for(size_t i = 0; i < count; ++i) {
        buffer[(h + i) % capacity] = data[i];
    }
    head.store((h + count) % capacity, std::memory_order_release);

    // track absolute decoded frames for track boundaries.
    // count is the number of floats; divide by 2 for stereo frames.
    absolute_frames_decoded.fetch_add(count / k_channels, std::memory_order_relaxed);

    return true;
}

size_t
audio_ring_buffer::pop(float* dest, size_t count) {
    // what do h and t contain here?
    size_t t = tail.load(std::memory_order_relaxed);
    size_t h = head.load(std::memory_order_acquire);
    
    // available what? to read what?
    size_t available = (h + capacity - t) % capacity; // available samples ready to be consumed
    size_t to_read = std::min(count, available); // samples we'll read this pop

    // apply the recalculated linear volume multiplier
    float current_vol = volume_multiplier.load(std::memory_order_relaxed);

    // calculate how many elements we can read in a row before reaching the buffer end
    size_t first_chunk = std::min(to_read, capacity - t);
    size_t second_chunk = to_read - first_chunk;

    float* __restrict__ d = dest;
    const float* __restrict__ b = buffer.data();

    // loop 1: read in a single pass till the buffer end (or until finishing to read)
    #pragma omp simd // compiler hint
    for (size_t i = 0; i < first_chunk; ++i) {
        d[i] = b[t + i] * current_vol; 
    }

    // if anything was left, wrap around and read from the beginning (index 0)
    if (second_chunk > 0) {
        #pragma omp simd
        for (size_t i = 0; i < second_chunk; ++i) {
            d[first_chunk + i] = b[i] * current_vol;
        }
    }

    // update the tail only once at the end
    tail.store((t + to_read) % capacity, std::memory_order_release);
    
    return to_read;
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

    // How much data is currently stored
    size_t used = (h + capacity - t) % capacity;

    // Total usable space is capacity - 1 (one slot reserved to distinguish empty from full)
    return (capacity - 1) - used;
}

// have we crossed yet?
bool
audio_ring_buffer::check_for_boundary_and_advance()
{
    std::lock_guard<std::mutex> lock(boundary_mutex);
    if (has_upcoming_boundary) {
        if (absolute_frames_played.load(std::memory_order_relaxed) >= upcoming_boundary_frame) {
            
            // Invalidate the boundary since we just crossed it
            has_upcoming_boundary = false;
            
            return true;
        }
    }
    return false;
}

uint64_t
audio_ring_buffer::playback_position_ms() const
{
    uint64_t frames  = frames_played.load(std::memory_order_relaxed);
    uint64_t base_ms = playback_base_ms.load(std::memory_order_relaxed);

    if (sample_rate == 0) return base_ms; // stream not open yet

    return base_ms + (frames * 1000) / sample_rate;
}

uint64_t
audio_ring_buffer::get_sample_rate () const
{
    return sample_rate;
}

std::chrono::microseconds
audio_ring_buffer::nap_for (size_t missing_samples) const
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    // Device not draining: the ring is full, so there are seconds of audio of slack.
    if (is_paused.load(std::memory_order_relaxed)) return 250ms;

    // Time the device needs to consume the missing space.
    const microseconds drain(static_cast<int64_t>(missing_samples * 1'000'000 / (sample_rate * k_channels)));

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