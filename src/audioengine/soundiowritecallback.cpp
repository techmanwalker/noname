#include "audioengine.hpp"
#include "audiointernalcontroller.hpp"
#include "audioringbuffer.hpp"

#include <soundio/soundio.h>

/**
    @brief Function callback for libsndio.

    @details Executed at a very high priority.
    Must never block, nor assign memory (no new nor std::vector),
    nor print to console. 
*/

void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    // fetch our buffer from the userdata pointer
    audio_ring_buffer *ring_buf = static_cast<audio_ring_buffer*>(outstream->userdata);
    
    int frames_left = frame_count_max;

    // Evaluate handshake status at the start of the callback
    bool needs_flush = ring_buf->flush_request.load(std::memory_order_acquire);
    bool is_seeking = ring_buf->pending_seeks.load(std::memory_order_acquire) > 0;
    
    // Acknowledge the flush request to the decoder thread
    if (needs_flush) {
        ring_buf->flush_ack.store(true, std::memory_order_release);
    } else {
        ring_buf->flush_ack.store(false, std::memory_order_release);
    }
    
    // remaining frames yet to decode
    while (frames_left > 0) {
        int frame_count = frames_left;
        struct SoundIoChannelArea *areas;
        
        // ask hardware to make room in memory
        if (execute_soundio(soundio_outstream_begin_write,
                outstream,
                &areas,
                &frame_count
            ) != 0) {
                break;
            }

        if (frame_count == 0) break; // no room, exit

        // If a seek is in progress, flush is requested, or transport is paused, feed silence.
        if (needs_flush || is_seeking || ring_buf->is_paused.load(std::memory_order_acquire)) {
            for (int frame = 0; frame < frame_count; ++frame) {
                for (int ch = 0; ch < outstream->layout.channel_count; ++ch) {
                    float *ptr = (float*)(areas[ch].ptr + areas[ch].step * frame);
                    *ptr = 0.0f; 
                }
            }
        } else {
            // Not paused, normal execution
            
            // Assign a temporal buffer on stack to be really fast
            // 8192 should be enough for any standard audio request (4096 frames * 2 channels)
            float temp_buf[8192] = {0.0f}; 
            
            // Extract interleaved data (L-R-L-R) from our ring buffer
            size_t floats_to_read = frame_count * outstream->layout.channel_count;
            size_t floats_read = ring_buf->pop(temp_buf, floats_to_read);

            // detect true end of playback
            if (floats_read < floats_to_read && ring_buf->eof_decoded.load(std::memory_order_acquire)) {
                if (!ring_buf->eof_played.exchange(true)) {
                    ring_buf->is_paused.store(true, std::memory_order_release);
                    QMetaObject::invokeMethod(&audio_engine::instance(), 
                                            &audio_engine::process_playlist_finished, 
                                            Qt::QueuedConnection);
                }
            }
            
            // libsoundio requires us to fill the channels using our own pointers (areas)
            int float_idx = 0;
            for (int frame = 0; frame < frame_count; ++frame) {
                for (int ch = 0; ch < outstream->layout.channel_count; ++ch) {
                    float *ptr = (float*)(areas[ch].ptr + areas[ch].step * frame);
                    
                    if (float_idx < floats_read) {
                        *ptr = temp_buf[float_idx++];
                    } else {
                        *ptr = 0.0f; // if no data, add silence
                    }
                }
            }

            /* These frame_count frames are now committed to the hardware -
               real elapsed playback time, silence-padded underrun or not.
               Lock-free atomic add: safe for the RT callback. */
            ring_buf->frames_played.fetch_add(static_cast<uint64_t>(frame_count),
                                               std::memory_order_relaxed);

            //  Track absolute played frames to trigger gapless UI updates
            uint64_t played = ring_buf->absolute_frames_played.fetch_add(
                    static_cast<uint64_t>(frame_count), std::memory_order_relaxed) + frame_count;

            uint64_t boundary_target = ring_buf->next_boundary_frame.load(std::memory_order_acquire);

            if (boundary_target != UINT64_MAX && played >= boundary_target) {
                // Disarm target so it fires only once
                ring_buf->next_boundary_frame.store(UINT64_MAX, std::memory_order_release);

                // Notify main thread reactively
                QMetaObject::invokeMethod(&audio_engine::instance(), 
                                        &audio_engine::process_track_boundary, 
                                        Qt::QueuedConnection);
            }

        }
        
        execute_soundio(soundio_outstream_end_write, outstream);
        frames_left -= frame_count;
    }
}


// Error handling

void error_callback(struct SoundIoOutStream *outstream, int err)
{
    qCWarning(l_soundio) << "SoundIo outstream error:" << soundio_strerror(err);
}

void underflow_callback (struct SoundIoOutStream *outstream)
{
    qCWarning(l_soundio) << "SoundIo outstream underflow";
};