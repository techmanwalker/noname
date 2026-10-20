#include "audiodecodeworker.hpp"
#include "audioringbuffer.hpp"
#include "l_audioengine.hpp"

#include <cerrno>
#include <optional>

#include <QLoggingCategory>
#include <QThread>

extern "C" {
#include <libavutil/frame.h>
}

namespace {
// Formats swr can produce directly. s24 = S32 >> 8, *_be = bswap, s8/companded: post-steps
// that don't exist yet. They fail loudly at open instead of playing garbage.
std::optional<AVSampleFormat>
to_ffmpeg (sample_format f)
{
    switch (f) {
    case sample_format::u8: return AV_SAMPLE_FMT_U8;
    case s16ne:             return AV_SAMPLE_FMT_S16;
    case s32ne:             return AV_SAMPLE_FMT_S32;
    case f32ne:             return AV_SAMPLE_FMT_FLT;
    case f64ne:             return AV_SAMPLE_FMT_DBL;
    default:                return std::nullopt;
    }
}
}

// The Decoding workhorse

audio_decode_worker::audio_decode_worker (stream_format out_format, QObject *parent)
    : QObject(parent),
      m_ring_buffer(out_format)
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
audio_decode_worker::load(const QUrl &source) {
    // The decoder is already on this exact source: rewind instead of reopening.
    if (m_song_loaded && fmt_ctx && source == m_open_source) {
        undo_prepare_B();   // load designates a new A: the B prepared for the old one goes, like clean()
        if (rewind_in_place(0)) return;
        // seek failed: fall through to a full reopen
    }

    // clear any previous context
    clean();

    // Open the new file using the helper
    if (!open_file_internal(source)) {
        m_song_loaded = false;
    }
}

bool
audio_decode_worker::open_file_internal(const QUrl &source)
{
    // FFmpeg is opened by filesystem path below, so only local files can be decoded for now.
    // NOTE: to support reading from external sources (http, remote shares, ...) the data has
    // to be brought into memory BEFORE decoding: either a temporary file copy, or some other
    // way to load it to memory (e.g. a custom AVIOContext over a QBuffer), then decode from that.
    if (!source.isLocalFile()) {
        qCCritical(l_ffmpeg) << "Cannot decode a non-local source:" << source;
        QString err_q = "Only local files can be decoded.";
        emit song_load_failed(err_q);
        return false;
    }
    const QString file_path = source.toLocalFile();

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

        const stream_format &out = m_ring_buffer.format();
        const std::optional<AVSampleFormat> out_av = to_ffmpeg(out.sample);

        AVChannelLayout out_ch_layout {};
        av_channel_layout_default(&out_ch_layout, out.channels);
        const AVChannelLayout &in_ch_layout = codec_ctx->ch_layout;

        // Convert only what differs. swr is skipped entirely when sample format (incl. packed
        // vs planar), rate and layout already match: frames go to the ring untouched.
        m_out_av_fmt  = out_av.value_or(AV_SAMPLE_FMT_NONE);
        m_passthrough = out_av
            && codec_ctx->sample_fmt == *out_av
            && uint32_t(codec_ctx->sample_rate) == out.rate
            && av_channel_layout_compare(&in_ch_layout, &out_ch_layout) == 0;

        int ret = out_av ? 0 : AVERROR(ENOTSUP);
        if (ret == 0 && !m_passthrough) {
            swr_ctx = nullptr;
            ret = swr_alloc_set_opts2(&swr_ctx,
                                      &out_ch_layout, *out_av, int(out.rate),
                                      &in_ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
                                      0, nullptr);
            if (ret == 0) ret = swr_init(swr_ctx);
        }
        av_channel_layout_uninit(&out_ch_layout);

        if (ret < 0) {
            char err_buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err_buf, AV_ERROR_MAX_STRING_SIZE);
            qCCritical(l_ffmpeg) << "Cannot convert this track to the ring format:" << err_buf;

            close_ffmpeg_contexts();

            QString err_q = QString::fromUtf8(err_buf);
            emit song_load_failed(err_q);
            return false;
        }

        m_open_source = source;
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
                // Seek/stop cancelled the push: bail out, they will reset codec and ring.
                const bool pushed = push_frame(frame);
                av_frame_unref(frame);

                if (!pushed) {
                    av_packet_free(&pkt);
                    av_frame_free(&frame);
                    return;
                }
            }
        }

        // make instantaneous, there might be more to read right now
        m_decode_timer->setInterval(0);

    } else if (read_ret == AVERROR_EOF && !m_ring_buffer.eof_decoded.load()) {
        avcodec_send_packet(codec_ctx, nullptr);
        while (avcodec_receive_frame(codec_ctx, frame) == 0) {
            const bool pushed = push_frame(frame);
            av_frame_unref(frame);

            // Cancelled mid-drain: a seek/stop/load is taking over and will reset the codec and
            // the ring. Do NOT fall through to step 2 below (arming the boundary, opening B's
            // file): the last samples of A's file never reached the ring.
            if (!pushed) {
                av_packet_free(&pkt);
                av_frame_free(&frame);
                return;
            }
        }

        // Rate conversion keeps a tail inside swr: flush it or every track loses its last few ms
        if (swr_ctx) {
            uint8_t *tail = nullptr;
            const int cap = swr_get_out_samples(swr_ctx, 0);
            if (cap > 0 && av_samples_alloc(&tail, nullptr, m_ring_buffer.format().channels,
                                            cap, m_out_av_fmt, 0) >= 0) {
                const int got = swr_convert(swr_ctx, &tail, cap, nullptr, 0);
                if (got > 0 && !m_ring_buffer.push_blocking(reinterpret_cast<const std::byte *>(tail), size_t(got))) {
                    av_freep(&tail);
                    av_packet_free(&pkt);
                    av_frame_free(&frame);
                    return;
                }
                av_freep(&tail);
            }
        }

        // 2. End of A's file: if B is prepared, open B's file and keep decoding right behind A's samples
        std::lock_guard<std::mutex> B_lock(m_B_mutex);

        if (m_has_B) {

            // Register the boundary between A and B for the UI
            uint64_t boundary_frame = m_ring_buffer.absolute_frames_decoded.load(std::memory_order_relaxed);
            {
                std::lock_guard<std::mutex> boundary_lock(m_ring_buffer.boundary_mutex);
                m_ring_buffer.upcoming_boundary_frame = boundary_frame;
                m_ring_buffer.has_upcoming_boundary = true;
            }

            // Arm the atomic tracker. An unconditional store is safe here since we strictly track one boundary at a time.
            m_ring_buffer.next_boundary_frame.store(boundary_frame, std::memory_order_release);

            // Remember both files: seeks while armed need A's, on_boundary_hit() needs to know B's
            m_track_A_source = m_open_source;       // A's file (close_ffmpeg_contexts() clears m_open_source)
            m_track_B_opened = m_track_B_source;    // B's file (m_B_mutex is held here)

            // The decoder's single open file moves from A's to B's
            close_ffmpeg_contexts();

            if (open_file_internal(m_track_B_source)) {
                m_has_B = false;   // consumed: B's file is open now

                // Immediately loop again to start decoding B
                m_decode_timer->setInterval(0);
            } else {
                // B's file failed to open, handle gracefully. Nothing is decodable now:
                // without this, the next decode_step() reads from a null/half-open context.
                close_ffmpeg_contexts();
                m_track_B_opened.clear();   // B never existed: the hit must report "no B"
                m_song_loaded.store(false);
                m_ring_buffer.eof_decoded.store(true, std::memory_order_release);
            }
        } else {
            // No B prepared: the end of A's file is where the tape runs out, stop decoding
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

bool
audio_decode_worker::rewind_in_place(uint64_t position_ms, std::optional<uint64_t> epoch)
{
    // Re-arm for normal operation before taking the lock and decoding new frames
    m_ring_buffer.reset_cancel();

    // decode and seek are mutually exclusive
    std::lock_guard<std::mutex> lock (m_decoder_mutex);

    // Scope ends before seek() decrements pending_seeks, as before
    audio_ring_buffer::rt_quiesce quiesce(m_ring_buffer);

    if (!fmt_ctx) return false;

    if (epoch) {
        using phase = audio_ring_buffer::boundary_phase;
        const auto b = m_ring_buffer.boundary_snapshot();   // RT is held out: nothing here can move under us

        // The request was computed against the A the engine presented when it was issued. If that
        // hit was consumed since (epoch moved) or the RT thread is already past the boundary
                // (phase hit), A is gone: applying the target to the file open now (B's) IS the cross-seek
        // bug. Drop it.
        if (b.epoch != *epoch || b.phase == phase::hit) return false;

        if (b.phase == phase::armed) {   // A is still audible, the decoder already reads B's file
            if (m_ring_buffer.skip_within_armed_tail(quiesce, position_ms)) return true;
            return reopen_A_at(quiesce, position_ms);
        }
    }
    return seek_and_reset(quiesce, position_ms);
}

bool
audio_decode_worker::seek_and_reset (const audio_ring_buffer::rt_quiesce &quiesce, uint64_t position_ms)
{
    const int64_t seek_target = av_rescale_q(position_ms, {1, 1000},
                                             fmt_ctx->streams[audio_stream_index]->time_base);

    if (av_seek_frame(fmt_ctx, audio_stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0)
        return false;                       // ring and eof flags untouched

    avcodec_flush_buffers(codec_ctx);
    if (swr_ctx) swr_init(swr_ctx);         // flush the resampler cache: no old audio spillage

    m_ring_buffer.reset(quiesce, position_ms);
    return true;
}

bool
audio_decode_worker::reopen_A_at (const audio_ring_buffer::rt_quiesce &quiesce, uint64_t position_ms)
{
    const QUrl B_source = m_open_source;
    const QUrl A_source = m_track_A_source;
    close_ffmpeg_contexts();

    if (open_file_internal(A_source) && seek_and_reset(quiesce, position_ms)) {
        std::lock_guard<std::mutex> lock(m_B_mutex);
        m_track_B_source = B_source;   // B's file is opened again when the decoder reaches the end of A's
        m_has_B          = true;
        return true;
    }

    // Same policy as when B's file fails to open: nothing decodable, the ring plays out what it holds
    close_ffmpeg_contexts();
    m_song_loaded.store(false);
    m_ring_buffer.eof_decoded.store(true, std::memory_order_release);
    return false;
}

void
audio_decode_worker::seek(uint64_t position_ms, uint64_t epoch)
{
    rewind_in_place(position_ms, epoch);

    // mark seek as complete so actual audio playback can safely resume
    m_ring_buffer.pending_seeks.fetch_sub(1, std::memory_order_release);

    // Exactly one completion per request, whatever happened to it
    emit seeked();
}

void
audio_decode_worker::prepare_B(const QUrl &source)
{
    std::lock_guard<std::mutex> lock(m_B_mutex);
    m_track_B_source = source;
    m_has_B = true;
}

void
audio_decode_worker::undo_prepare_B()
{
    std::lock_guard<std::mutex> lock(m_B_mutex);
    m_has_B = false;
    m_track_B_source.clear();
}

int
audio_decode_worker::convert_to_ring (AVFrame *frame, uint8_t **output)
{
    const stream_format &out = m_ring_buffer.format();

    const int dst_frames = av_rescale_rnd(
        swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
        out.rate, frame->sample_rate, AV_ROUND_UP);

    uint8_t *converted = nullptr;
    int ret = av_samples_alloc(&converted, nullptr, out.channels, dst_frames, m_out_av_fmt, 0);
    if (ret < 0) return ret;

    const int got = swr_convert(swr_ctx, &converted, dst_frames,
                                (const uint8_t **)frame->data, frame->nb_samples);
    if (got < 0) { av_freep(&converted); return got; }

    *output = converted;
    return got; // frames: no more "* 2"
}

bool
audio_decode_worker::push_frame (AVFrame *frame)
{
    if (m_passthrough)
        return m_ring_buffer.push_blocking(reinterpret_cast<const std::byte *>(frame->data[0]),
                                           size_t(frame->nb_samples));

    uint8_t *pcm = nullptr;
    const int frames = convert_to_ring(frame, &pcm);
    if (frames <= 0) { av_freep(&pcm); return true; } // nothing to push is not a cancel

    const bool ok = m_ring_buffer.push_blocking(reinterpret_cast<const std::byte *>(pcm), size_t(frames));
    av_freep(&pcm);
    return ok;
}

void
audio_decode_worker::clean ()
{
    // ffmpeg clean

    close_ffmpeg_contexts();
    undo_prepare_B();   // B was prepared to follow what we just closed. Not ring state: kept out of the quiesce window

    {
        audio_ring_buffer::rt_quiesce quiesce(m_ring_buffer);
        m_ring_buffer.reset(quiesce, 0);
    }

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
    m_open_source.clear();
}

QUrl
audio_decode_worker::track_B_opened () const
{
    std::lock_guard<std::mutex> lock(m_B_mutex);
    return m_track_B_opened;
}