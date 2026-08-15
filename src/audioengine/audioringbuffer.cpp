#include "audioringbuffer.hpp"

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
    
    m_cv.wait(lock, [this, count] {
        return m_cancelled.load() || writable_size() >= count;
    });
    
    if (m_cancelled.load()) return false;
    
    // guaranteed to succeed - do the push
    size_t h = head.load(std::memory_order_relaxed);
    for(size_t i = 0; i < count; ++i) {
        buffer[(h + i) % capacity] = data[i];
    }
    head.store((h + count) % capacity, std::memory_order_release);

    // track absolute decoded frames for track boundaries.
    // count is the number of floats; divide by 2 for stereo frames.
    absolute_frames_decoded.fetch_add(count / 2, std::memory_order_relaxed);

    return true;
}

size_t
audio_ring_buffer::pop(float* dest, size_t count) {
    // what do h and t contain here?
    size_t t = tail.load(std::memory_order_relaxed);
    size_t h = head.load(std::memory_order_acquire);
    
    // available what? to read what?
    size_t available = (h + capacity - t) % capacity;
    size_t to_read = std::min(count, available);

    // apply the recalculated linear volume multiplier
    float current_vol = volume_multiplier.load(std::memory_order_relaxed);

    for(size_t i = 0; i < to_read; ++i) {
        dest[i] = buffer[(t + i) % capacity] * current_vol;
    };
    
    tail.store((t + to_read) % capacity, std::memory_order_release);

    // wake the decoder if it was blocked waiting for space
    if (to_read > 0) {
        m_cv.notify_one();
    }

    return to_read;
}

void
audio_ring_buffer::cancel_blocking_push()
{
    m_cancelled.store(true);
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