#pragma once

#include <atomic>
#include <condition_variable>
#include <optional>
#include <vector>

// sndio
void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);

void error_callback(struct SoundIoOutStream *outstream, int err);

void underflow_callback (struct SoundIoOutStream *outstream);

class audio_ring_buffer
{
    friend class audio_decode_worker;

    const uint64_t sample_rate; // by default

    size_t capacity = sample_rate * 10 * 2; // ~5s of stereo audio

    // not deque so it does not move
    std::vector<float> buffer;

    std::atomic<size_t> head{0}; // where FFmpeg writes
    std::atomic<size_t> tail{0}; // where libsoundio reads

    // synchronization for a blocking push, fundamental to avoid skim decoding
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic_bool m_cancelled{false};

    // internal push to ring buffer
    bool __push_unlocked (const float* data, size_t count);

public:

    audio_ring_buffer (std::optional<size_t> requested_sample_rate = std::nullopt);

    // called by the ffmpeg thread to write to the output sound pipe
    // blocking push - guaranteed to succeed unless cancelled
    bool push_blocking(const float* data, size_t count);

    // cancel an in-flight blocking push (call before seek/reset)
    void cancel_blocking_push();

    // re-arm after cancel
    void reset_cancel();

    // called by libsoundio write_callback
    size_t pop (float* dest, size_t count);

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
    
    // Handshake sequence to safely reset head/tail without clobbering
    std::atomic_bool flush_request{false};
    std::atomic_bool flush_ack{false};

    std::mutex boundary_mutex;
    bool has_upcoming_boundary{false};
    uint64_t upcoming_boundary_frame{0};

    // Target frame count for the next upcoming track boundary (UINT64_MAX when none)
    std::atomic_uint64_t next_boundary_frame{UINT64_MAX};

    bool check_for_boundary_and_advance();
};