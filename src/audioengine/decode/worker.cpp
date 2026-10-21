#include "decode/worker.hpp"
#include "ringbuffer/ringbuffer.hpp"
#include "logging/l_audioengine.hpp"

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

// --- timer pacing (ms). decode_step is the only place that applies them.

namespace pace_ms {
constexpr int spin     = 0;    // more to read right now
constexpr int backoff  = 10;   // ring full / transient error
constexpr int no_song  = 50;
constexpr int finished = 100;  // A's tape ended, nothing to decode
}

// --- RAII at the FFmpeg boundary: no manual free on any exit path

struct av_buffer_deleter { void operator() (uint8_t *b) const { av_free(b); } };
using av_buffer_ptr = std::unique_ptr<uint8_t, av_buffer_deleter>; // packed samples: one block

struct default_layout
{
    AVChannelLayout value {};

    explicit default_layout (int channels) { av_channel_layout_default(&value, channels); }
    ~default_layout () { av_channel_layout_uninit(&value); }

    default_layout (const default_layout &) = delete;
    default_layout &operator= (const default_layout &) = delete;
};

// --- open pipeline
// Each stage: nullopt = ok, otherwise the reason. Stages never log, never close, never emit.

struct open_error { QString message; };
using open_status = std::optional<open_error>;

QString
av_error_text (int ret)
{
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, buf, sizeof buf);
    return QString::fromUtf8(buf);
}

open_error
av_failure (int ret, const char *what)
{
    return { QStringLiteral("%1: %2").arg(QString::fromUtf8(what), av_error_text(ret)) };
}

// How decoded frames reach the ring: untouched, or through swr.
struct conversion_plan
{
    AVSampleFormat out_fmt     = AV_SAMPLE_FMT_NONE;
    bool           passthrough = false;
};

// Borrowed views of the worker's FFmpeg pointers: the stages are free functions with no class access.
struct open_ctx
{
    AVFormatContext     *&fmt;
    AVCodecContext      *&codec;
    SwrContext          *&swr;
    int                  &stream_index;
    const stream_format  &out;
    conversion_plan       plan {};
};

open_status
require_local_file (const QUrl &source)
{
    // FFmpeg is opened by filesystem path, so only local files can be decoded for now.
    // NOTE: to support reading from external sources (http, remote shares, ...) the data has
    // to be brought into memory BEFORE decoding: either a temporary file copy, or some other
    // way to load it to memory (e.g. a custom AVIOContext over a QBuffer), then decode from that.
    if (source.isLocalFile()) return std::nullopt;
    return open_error{ QStringLiteral("Only local files can be decoded.") };
}

open_status
open_container (open_ctx &c, const QString &path)
{
    if (const int ret = avformat_open_input(&c.fmt, path.toUtf8().constData(), nullptr, nullptr); ret < 0)
        return av_failure(ret, "Cannot open file");

    if (const int ret = avformat_find_stream_info(c.fmt, nullptr); ret < 0)
        return av_failure(ret, "Cannot read stream info");

    return std::nullopt;
}

open_status
select_audio_stream (open_ctx &c)
{
    c.stream_index = av_find_best_stream(c.fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (c.stream_index >= 0) return std::nullopt;
    return open_error{ QStringLiteral("File does not contain a valid audio stream.") };
}

open_status
open_decoder (open_ctx &c)
{
    const AVCodecParameters *params = c.fmt->streams[c.stream_index]->codecpar;

    const AVCodec *codec = avcodec_find_decoder(params->codec_id);
    if (!codec) return open_error{ QStringLiteral("No decoder available for this codec.") };

    c.codec = avcodec_alloc_context3(codec);
    if (!c.codec) return av_failure(AVERROR(ENOMEM), "Cannot allocate decoder");

    if (const int ret = avcodec_parameters_to_context(c.codec, params); ret < 0)
        return av_failure(ret, "Cannot configure decoder");

    if (const int ret = avcodec_open2(c.codec, codec, nullptr); ret < 0)
        return av_failure(ret, "Cannot open decoder");

    return std::nullopt;
}

// Convert only what differs. swr is skipped entirely when sample format (incl. packed
// vs planar), rate and layout already match: frames go to the ring untouched.
open_status
plan_conversion (open_ctx &c)
{
    const std::optional<AVSampleFormat> out_av = to_ffmpeg(c.out.sample);
    if (!out_av)
        return av_failure(AVERROR(ENOTSUP), "Cannot convert this track to the ring format");

    const default_layout out_layout(c.out.channels);

    c.plan.out_fmt     = *out_av;
    c.plan.passthrough = c.codec->sample_fmt == *out_av
        && uint32_t(c.codec->sample_rate) == c.out.rate
        && av_channel_layout_compare(&c.codec->ch_layout, &out_layout.value) == 0;

    return std::nullopt;
}

open_status
init_resampler (open_ctx &c)
{
    if (c.plan.passthrough) return std::nullopt;

    const default_layout out_layout(c.out.channels);

    c.swr = nullptr;
    int ret = swr_alloc_set_opts2(&c.swr,
                                  &out_layout.value, c.plan.out_fmt, int(c.out.rate),
                                  &c.codec->ch_layout, c.codec->sample_fmt, c.codec->sample_rate,
                                  0, nullptr);
    if (ret == 0) ret = swr_init(c.swr);

    if (ret < 0) return av_failure(ret, "Cannot convert this track to the ring format");
    return std::nullopt;
}

// The whole open sequence: failures are early returns, success is the straight line.
open_status
open_pipeline (open_ctx &c, const QUrl &source)
{
    if (auto e = require_local_file(source))               return e;
    if (auto e = open_container(c, source.toLocalFile()))  return e;
    if (auto e = select_audio_stream(c))                   return e;
    if (auto e = open_decoder(c))                          return e;
    if (auto e = plan_conversion(c))                       return e;
    return init_resampler(c);
}

// --- PCM blocks: ring-format samples owned by RAII, so no path can leak or double free one

struct pcm_block
{
    av_buffer_ptr data;
    int           frames = 0;   // frames valid in `data` (0 = nothing to push)
    int           error  = 0;   // AVERROR code when alloc/convert failed, else 0
};

// Room for `capacity` frames, then swr over `in` (nullptr/0 = flush its tail).
// The only place samples are allocated, hence the only place that frees them.
pcm_block
convert_block (SwrContext *swr, int channels, AVSampleFormat out_fmt, int capacity,
               const uint8_t **in, int in_frames)
{
    pcm_block block;

    uint8_t *raw = nullptr;
    if (const int ret = av_samples_alloc(&raw, nullptr, channels, capacity, out_fmt, 0); ret < 0) {
        block.error = ret;
        return block;
    }
    block.data.reset(raw);

    const int got = swr_convert(swr, &raw, capacity, in, in_frames);
    if (got < 0) {
        block.data.reset();     // frees
        block.error = got;
        return block;
    }

    block.frames = got;
    return block;
}

// A decoded frame -> ring format, through swr.
pcm_block
convert_frame (SwrContext *swr, const stream_format &out, AVSampleFormat out_fmt, AVFrame &frame)
{
    const int capacity = int(av_rescale_rnd(swr_get_delay(swr, frame.sample_rate) + frame.nb_samples,
                                            out.rate, frame.sample_rate, AV_ROUND_UP));
    return convert_block(swr, out.channels, out_fmt, capacity,
                         const_cast<const uint8_t **>(frame.data), frame.nb_samples);
}

// The single bytes-to-ring seam (the reinterpret_cast used to be written three times).
bool
push_pcm (audio_ring_buffer &ring, const uint8_t *samples, int frames)
{
    return ring.push_blocking(reinterpret_cast<const std::byte *>(samples), size_t(frames));
}

// --- decode step

enum class drain_status { complete, cancelled };

// Push every frame the codec has ready. `cancelled`: a seek/stop/load cancelled the push and
// is taking over; it will reset the codec and the ring, so the caller must bail out.
template <typename Push>
drain_status
drain_frames (AVCodecContext *codec, AVFrame *frame, Push &&push)
{
    while (avcodec_receive_frame(codec, frame) == 0) {
        const bool pushed = push(frame);
        av_frame_unref(frame);
        if (!pushed) return drain_status::cancelled;
    }
    return drain_status::complete;
}

// pkt == nullptr puts the codec in draining mode (end of file).
// A packet the codec refuses is dropped and logged, NOT fatal: one corrupt frame in the middle
// of a file must not stop the track. Whatever the codec still holds is drained either way.
template <typename Push>
drain_status
feed_codec (AVCodecContext *codec, const AVPacket *pkt, AVFrame *frame, Push &&push)
{
    int ret = avcodec_send_packet(codec, pkt);

    // EAGAIN: output not drained yet and the packet was NOT consumed. Drain, then it is accepted.
    if (ret == AVERROR(EAGAIN)) {
        if (drain_frames(codec, frame, push) == drain_status::cancelled)
            return drain_status::cancelled;
        ret = avcodec_send_packet(codec, pkt);
    }

    // EOF on a flush just means "already draining": nothing to report
    const bool already_draining = !pkt && ret == AVERROR_EOF;
    if (ret < 0 && !already_draining)
        qCWarning(l_ffmpeg) << "Codec rejected its input:" << av_error_text(ret);

    return drain_frames(codec, frame, push);
}

// Rate conversion keeps a tail inside swr: flush it or every track loses its last few ms.
drain_status
flush_resampler (SwrContext *swr, audio_ring_buffer &ring, AVSampleFormat out_fmt)
{
    if (!swr) return drain_status::complete;

    const int cap = swr_get_out_samples(swr, 0);
    if (cap <= 0) return drain_status::complete;

    const pcm_block tail = convert_block(swr, ring.format().channels, out_fmt, cap, nullptr, 0);

    if (tail.frames > 0 && !push_pcm(ring, tail.data.get(), tail.frames))
        return drain_status::cancelled;
    return drain_status::complete;
}

// nullopt = go decode; otherwise the interval to wait. Order matters: no song wins over full ring.
std::optional<int>
idle_interval (bool song_loaded, audio_ring_buffer &ring)
{
    if (!song_loaded)    return pace_ms::no_song;
    if (ring.is_full())  return pace_ms::backoff;   // don't decode ahead and waste RAM
    return std::nullopt;
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

    // Failure is fully handled inside: contexts closed, song unloaded, song_load_failed emitted
    open_file_internal(source);
}

bool
audio_decode_worker::open_file_internal(const QUrl &source)
{
    // The stages need a clean slate (avformat_open_input would adopt a live fmt_ctx, init_resampler
    // would leak swr_ctx). Every caller closes first; this makes it a checked contract.
    Q_ASSERT(!fmt_ctx && !codec_ctx && !swr_ctx);
    open_ctx ctx { fmt_ctx, codec_ctx, swr_ctx, audio_stream_index, m_ring_buffer.format() };

    if (open_status failure = open_pipeline(ctx, source)) {
        qCCritical(l_ffmpeg) << "Cannot open" << source << "-" << failure->message;
        abandon_decoding(true);     // whatever stage failed, no half-open context survives
        emit song_load_failed(source, std::move(failure->message));
        return false;
    }

    m_out_av_fmt  = ctx.plan.out_fmt;
    m_passthrough = ctx.plan.passthrough;
    m_open_source = source;
    m_song_loaded = true;
    return true;
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

    // The file ended and its end is waiting for the boundary before it: nothing to read, and a full
    // ring is expected. boundary_settled() resumes, so this is not polling for the consume.
    if (m_end_deferred) {
        m_decode_timer->setInterval(pace_ms::finished);
        return;
    }

    if (const auto idle = idle_interval(m_song_loaded, m_ring_buffer)) {
        m_decode_timer->setInterval(*idle);
        return;
    }

    // Both are empty here, and emptied again on every way out of this function
    const auto scratch = m_scratch.acquire();
    AVPacket *const pkt   = scratch.pkt();
    AVFrame  *const frame = scratch.frame();

    const auto push = [this] (AVFrame *f) { return push_frame(f); };

    const int read_ret = av_read_frame(fmt_ctx, pkt);

    // 1. A packet: decode it if it is ours (a packet can produce several frames)
    if (read_ret >= 0) {
        const bool ours = pkt->stream_index == audio_stream_index;
        if (ours && feed_codec(codec_ctx, pkt, frame, push) == drain_status::cancelled) {
            return;     // seek/stop cancelled the push: they reset codec and ring
        }

        m_decode_timer->setInterval(pace_ms::spin);   // there might be more to read right now
        return;
    }

    // 2. End of A's file, first time we see it
    if (read_ret == AVERROR_EOF && !m_ring_buffer.tape_ended()) {
        // Cancelled mid-drain: a seek/stop/load is taking over. Do NOT fall through to
        // on_end_of_A (arming the boundary, opening B's file): the last samples of A never
        // reached the ring.
        if (feed_codec(codec_ctx, nullptr, frame, push) == drain_status::cancelled
            || flush_resampler(swr_ctx, m_ring_buffer, m_out_av_fmt) == drain_status::cancelled)
            return;

        m_decode_timer->setInterval(on_end_of_A());
        return;
    }

    // 3. Generic error, or EOF already processed
    m_decode_timer->setInterval(pace_ms::backoff);
}

// A's file is fully in the ring. Either hand the decoder over to B, or declare the tape ended.
// Returns the timer interval for the next step.
int
audio_decode_worker::on_end_of_A ()
{
    // While A/B is still drawn (armed, or hit but not consumed) this file's end cannot be drawn: its
    // samples already fit in the ring, but the marker would overwrite A/B's. Decide nothing now:
    // boundary_settled() calls back, and the B slot is read then (a later prepare/undo wins).
    if (m_ring_buffer.boundary_snapshot().phase != audio_ring_buffer::boundary_phase::none) {
        m_end_deferred = true;
        return pace_ms::finished;
    }
    m_end_deferred = false;

    std::lock_guard<std::mutex> B_lock(m_B_mutex);

    if (!m_has_B) {
        // No B prepared: the end of A's file is where the tape runs out, stop decoding
        m_ring_buffer.mark_tape_ended();
        return pace_ms::finished;
    }

    m_ring_buffer.arm_boundary();

    // Remember both files: seeks while armed need A's, on_boundary_hit() needs to know B's
    m_track_A_source = m_open_source;       // close_ffmpeg_contexts() clears m_open_source
    m_track_B_opened = m_track_B_source;    // m_B_mutex is held here

    // The decoder's single open file moves from A's to B's
    close_ffmpeg_contexts();

    if (!open_file_internal(m_track_B_source)) {
        // open_file_internal reported the failure. B never existed: the hit must report "no B".
        m_track_B_opened.clear();
        abandon_decoding();
        return pace_ms::no_song;
    }

    m_has_B = false;            // consumed: B's file is open now
    return pace_ms::spin;       // start decoding B right behind A's samples
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
    m_end_deferred = false;                 // decoding resumes from here: the old file end is gone
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
    abandon_decoding();
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

void
audio_decode_worker::boundary_settled ()
{
    std::lock_guard<std::mutex> lock(m_decoder_mutex);
    if (!m_end_deferred) return;

    m_decode_timer->setInterval(on_end_of_A());   // phase is none now; the slot is what the engine left
}

bool
audio_decode_worker::push_frame (AVFrame *frame)
{
    if (m_passthrough)
        return push_pcm(m_ring_buffer, frame->data[0], frame->nb_samples);

    const pcm_block pcm = convert_frame(swr_ctx, m_ring_buffer.format(), m_out_av_fmt, *frame);
    if (pcm.error)
        qCWarning(l_ffmpeg) << "Cannot convert a frame to the ring format:" << av_error_text(pcm.error);

    if (pcm.frames <= 0) return true;   // nothing to push is not a cancel
    return push_pcm(m_ring_buffer, pcm.data.get(), pcm.frames);
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
    m_end_deferred = false;
}

void
audio_decode_worker::abandon_decoding (bool do_not_mark_end_of_tape)
{
    close_ffmpeg_contexts();
    m_song_loaded.store(false);
    if (!do_not_mark_end_of_tape) m_ring_buffer.mark_tape_ended();
}

QUrl
audio_decode_worker::track_B_opened () const
{
    std::lock_guard<std::mutex> lock(m_B_mutex);
    return m_track_B_opened;
}