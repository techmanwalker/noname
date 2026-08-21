#include "audiodecodeworker.hpp"
#include "audioringbuffer.hpp"
#include "l_audioengine.hpp"

#include <QLoggingCategory>
#include <QThread>

extern "C" {
#include <libavutil/frame.h>
}

// The Decoding workhorse

audio_decode_worker::audio_decode_worker (QObject *parent)
    : QObject(parent),
      m_ring_buffer() 
{
}

audio_decode_worker::~audio_decode_worker ()
{
    close_ffmpeg_contexts();
}

audio_ring_buffer*
audio_decode_worker::get_ring_buffer()
{
    return &m_ring_buffer;
}

bool
audio_decode_worker::is_song_loaded () const
{
    return m_song_loaded.load();
}

void 
audio_decode_worker::load(const QString &file_path) {
    // clear any previous context
    clean();

    // Open the new file using the helper
    if (!open_file_internal(file_path)) {
        m_song_loaded = false;
    }
}

bool
audio_decode_worker::open_file_internal(const QString &file_path)
{
    // 1. Open the format (the container)
    if (avformat_open_input(&fmt_ctx, file_path.toUtf8().constData(), nullptr, nullptr) < 0) {
        return false;
    }

    // 2. Read streams info
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        return false;
    }

    // 3. Find best audio stream
    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (audio_stream_index >= 0) {
        // 4. Setup decoder
        AVCodecParameters *params = fmt_ctx->streams[audio_stream_index]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(params->codec_id);
        codec_ctx = avcodec_alloc_context3(codec);
        
        avcodec_parameters_to_context(codec_ctx, params);
        avcodec_open2(codec_ctx, codec, nullptr);

        AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout in_ch_layout = codec_ctx->ch_layout;

        swr_ctx = nullptr; // reset
        int ret = swr_alloc_set_opts2(
            &swr_ctx,
            &out_ch_layout, AV_SAMPLE_FMT_FLT, get_ring_buffer()->sample_rate,
            &in_ch_layout,  codec_ctx->sample_fmt, codec_ctx->sample_rate, 
            0, nullptr
        );

        if (ret < 0) {
            char err_buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err_buf, AV_ERROR_MAX_STRING_SIZE);
            qCFatal(l_ffmpeg) << "Critical error in SwrContext: " << err_buf;

            close_ffmpeg_contexts();
            
            QString err_q = QString::fromUtf8(err_buf);
            emit song_load_failed(err_q);
            return false;
        }

        swr_init(swr_ctx);
        m_song_loaded = true;
        return true;
    } 
    
    // No valid audio stream found
    close_ffmpeg_contexts();
    QString err_q = "File does not contain a valid audio stream.";
    m_song_loaded = false;
    emit song_load_failed(err_q);
    
    return false;
}

void
audio_decode_worker::start_decoding()
{
    m_decode_timer = new QTimer(this);
    connect(m_decode_timer, &QTimer::timeout, this, &audio_decode_worker::decode_step);
    m_decode_timer->start(0);
}

void
audio_decode_worker::decode_step ()
{
    // decode and seek are mutually exclusive
    std::lock_guard<std::mutex> lock(m_decoder_mutex);

    // 1. Sleep if no song loaded
    if (!m_song_loaded) {
        m_decode_timer->setInterval(50);
        return;
    }

    // 2. Is the buffer full?
    // If it is, wait. We don't want to keep decoding and waste RAM.
    if (m_ring_buffer.is_full()) {
        m_decode_timer->setInterval(10);
        return;
    }

    // 3. FFmpeg decoding loop
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    int read_ret = av_read_frame(fmt_ctx, pkt);

    // if a packet was successfully read
    if (read_ret >= 0) {

        // is this the right stream?
        if (pkt->stream_index == audio_stream_index) {
            avcodec_send_packet(codec_ctx, pkt);
            
            // 4. receive next raw frame from file (mp3, flac, etc)
            //    a packet can produce more than a frame; drain
            while (avcodec_receive_frame(codec_ctx, frame) == 0) {

                // resample to match audio server expectations

                float *pcm_output;
                int samples_out = convert_to_float(frame, &pcm_output);
                
                // 5. Push to the ring buffer
                if (samples_out > 0 && pcm_output != nullptr) {
                    /* BLOCKING push: decoder thread sleeps here until
                       the audio callback drains enough buffer space.
                       Returns false only if seek cancelled us. */
                    bool pushed = m_ring_buffer.push_blocking(pcm_output, samples_out);

                    if (!pushed) {
                        // Seek cancelled this push — bail out cleanly.
                        // The seek will reset the codec and ring buffer.
                        av_freep(&pcm_output);
                        av_frame_unref(frame);
                        av_packet_free(&pkt);
                        av_frame_free(&frame);
                        return;
                    }
                }

                av_freep(&pcm_output);
                
                // clean for next use
                av_frame_unref(frame);
            }
        }

        // make instantaneous, there might be more to read right now
        m_decode_timer->setInterval(0);

    } else if (read_ret == AVERROR_EOF && !m_ring_buffer.eof_decoded.load()) {
        // 1. Send a flush packet and drain the remaining frames
        avcodec_send_packet(codec_ctx, nullptr);
        while (avcodec_receive_frame(codec_ctx, frame) == 0) {
            float *pcm_output;
            int samples_out = convert_to_float(frame, &pcm_output);
            if (samples_out > 0 && pcm_output != nullptr) {
                if (!m_ring_buffer.push_blocking(pcm_output, samples_out)) {
                    av_freep(&pcm_output);
                    break;
                }
            }
            av_freep(&pcm_output);
            av_frame_unref(frame);
        }

        // 2. GAPLESS HANDOFF CHECK
        std::lock_guard<std::mutex> q_lock(m_queue_mutex);

        if (m_has_queued_song) {

            // Register the boundary for the UI
            uint64_t boundary_frame = m_ring_buffer.absolute_frames_decoded.load(std::memory_order_relaxed);
            {
                std::lock_guard<std::mutex> b_lock(m_ring_buffer.boundary_mutex);
                m_ring_buffer.upcoming_boundary_frame = boundary_frame;
                m_ring_buffer.has_upcoming_boundary = true;
            }

            // Arm the atomic tracker. An unconditional store is safe here since we strictly track one boundary at a time.
            m_ring_buffer.next_boundary_frame.store(boundary_frame, std::memory_order_release);

            // Swap contexts transparently
            close_ffmpeg_contexts();

            if (open_file_internal(m_queued_next_song.source.toLocalFile())) {
                m_has_queued_song = false;
                
                // Immediately loop again to start decoding the new file
                m_decode_timer->setInterval(0); 
            } else {
                // Next file failed to open, handle gracefully
                m_ring_buffer.eof_decoded.store(true, std::memory_order_release);
            }
        } else {
            // No queued song, actually stop decoding
            m_ring_buffer.eof_decoded.store(true, std::memory_order_release);
            m_decode_timer->setInterval(100); 
        }
    } else {
        // generic error or eof already processed

        m_decode_timer->setInterval(10);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
}

void
audio_decode_worker::seek(uint64_t position_ms) {
    // If no context, abort and decrement the pending seek count
    if (!fmt_ctx) {
        m_ring_buffer.pending_seeks.fetch_sub(1, std::memory_order_release);
        m_ring_buffer.reset_cancel(); // Re-arm even if we abort
        return;
    }

    // reset eof
    m_ring_buffer.eof_decoded.store(false);
    m_ring_buffer.eof_played.store(false);

    // Re-arm for normal operation before taking the lock and decoding new frames
    m_ring_buffer.reset_cancel();
    
    // decode and seek are mutually exclusive
    std::lock_guard<std::mutex> lock (m_decoder_mutex);

    // hndshake with the RT thread to safely lock it into a flush state
    m_ring_buffer.flush_request.store(true, std::memory_order_release);
    while (!m_ring_buffer.flush_ack.load(std::memory_order_acquire)) {
        QThread::yieldCurrentThread(); 
    }

    // we are now in a safe zone where the RT thread will NOT touch head or tail

    // convert ms to FFmpeg timestamps
    int64_t seek_target = av_rescale_q(position_ms, 
                                        // time scale fraction
                                       {1, 1000}, 
                                       fmt_ctx->streams[audio_stream_index]->time_base);

    // perform the seek
    if (av_seek_frame(fmt_ctx, audio_stream_index, seek_target, AVSEEK_FLAG_BACKWARD) >= 0) {
        
        // Clear FFmpeg decoder buffers
        avcodec_flush_buffers(codec_ctx);
        
        // IMPORTANT: Flush the SwrContext resampler cache to prevent old audio spillage!
        if (swr_ctx) {
            swr_init(swr_ctx);
        }
        
        // Safely reset ring buffer (RT thread is paused waiting)
        m_ring_buffer.head.store(0, std::memory_order_relaxed);
        m_ring_buffer.tail.store(0, std::memory_order_relaxed);

        m_ring_buffer.playback_base_ms.store(position_ms, std::memory_order_relaxed);
        m_ring_buffer.frames_played.store(0, std::memory_order_relaxed);
        
        // Reset absolute gapless trackers for the new position
        m_ring_buffer.absolute_frames_decoded.store(0, std::memory_order_relaxed);
        m_ring_buffer.absolute_frames_played.store(0, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> b_lock(m_ring_buffer.boundary_mutex);
            m_ring_buffer.has_upcoming_boundary = false;
        }

        // Reset the atomic boundary target here
        m_ring_buffer.next_boundary_frame.store(UINT64_MAX, std::memory_order_relaxed);

        emit seeked();
    }

    // release the RT thread flush handshake
    m_ring_buffer.flush_request.store(false, std::memory_order_release);
    while (m_ring_buffer.flush_ack.load(std::memory_order_acquire)) {
        QThread::yieldCurrentThread();
    }

    // mark seek as complete so actual audio playback can safely resume
    m_ring_buffer.pending_seeks.fetch_sub(1, std::memory_order_release);
}

void
audio_decode_worker::queue_next_song(const Types::Song& next_song)
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_queued_next_song = next_song;
    m_has_queued_song = true;
}

void
audio_decode_worker::clear_queued_song()
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_has_queued_song = false;
    m_queued_next_song = Types::Song{};
}

int
audio_decode_worker::convert_to_float (AVFrame *frame, float **output)
{
    // Calculate needed frames for output
    int dst_nb_samples = av_rescale_rnd(
        swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
        get_ring_buffer()->get_sample_rate(), frame->sample_rate, AV_ROUND_UP
    );

    // Assign memory for the converted data
    uint8_t *converted_data = nullptr;
    av_samples_alloc(&converted_data, nullptr, 2, dst_nb_samples, AV_SAMPLE_FMT_FLT, 0);

    // Convert the audio to the standard format
    int samples_out = swr_convert(
        swr_ctx, &converted_data, dst_nb_samples,
        (const uint8_t **)frame->data, frame->nb_samples
    );

    // Assign the pointer to the float* the ring_buffer needs
    *output = (float*)converted_data;
    
    // NOTE: remember to free converted_data after doing push()
    // in the main thread with av_freep(&converted_data);
    return samples_out * 2; // * 2 for 2 channels (stereo)
}

void
audio_decode_worker::clean ()
{
    // ffmpeg clean

    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
    }
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }
    if (swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = nullptr;
    }

    // empty buffer
    m_ring_buffer.head.store (0);
    m_ring_buffer.tail.store (0);

    // reset playback time to 0
    m_ring_buffer.frames_played.store(0);
    m_ring_buffer.playback_base_ms.store(0);

    // reset absolute gapless trackers
    m_ring_buffer.absolute_frames_decoded.store(0, std::memory_order_relaxed);
    m_ring_buffer.absolute_frames_played.store(0, std::memory_order_relaxed);

    // invalidate the track boundary of the outgoing song
    {
        std::lock_guard<std::mutex> b_lock(m_ring_buffer.boundary_mutex);
        m_ring_buffer.has_upcoming_boundary = false;
    }
    m_ring_buffer.next_boundary_frame.store(UINT64_MAX, std::memory_order_relaxed);

    // clear any gapless handoff attached to the old song
    {
        std::lock_guard<std::mutex> q_lock(m_queue_mutex);
        m_has_queued_song = false;
        m_queued_next_song = Types::Song{};
    }

    // also reset eof state
    m_ring_buffer.eof_decoded.store(false);
    m_ring_buffer.eof_played.store(false);

    // mark no loaded song
    m_song_loaded.store(false);
}

void
audio_decode_worker::close_ffmpeg_contexts()
{
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
    }
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }
    if (swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = nullptr;
    }
}