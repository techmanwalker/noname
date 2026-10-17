#include "coverdecode.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
}

#include <QImage>

#include <algorithm>
#include <cstring>
#include <memory>

#include "pixelformats.hpp"

namespace covers::decode {

struct BufferData {
    const uint8_t *ptr;
    size_t size;
};

namespace {

struct AVIOCtxDeleter {
    void operator()(AVIOContext *ctx) const {
        if (ctx) { av_freep(&ctx->buffer); avio_context_free(&ctx); }
    }
};
struct AVFormatCtxDeleter {
    void operator()(AVFormatContext *ctx) const {
        if (ctx) avformat_close_input(&ctx);   // leaves our custom pb alone
    }
};
struct AVCodecCtxDeleter {
    void operator()(AVCodecContext *ctx) const {
        if (ctx) avcodec_free_context(&ctx);
    }
};
struct AVFrameDeleter  { void operator()(AVFrame  *f) const { if (f) av_frame_free(&f);  } };
struct AVPacketDeleter { void operator()(AVPacket *p) const { if (p) av_packet_free(&p); } };

int
read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    auto *bd = static_cast<BufferData *>(opaque);
    const size_t n = std::min(static_cast<size_t>(buf_size), bd->size);

    if (n == 0) {
        return AVERROR_EOF;
    }

    std::memcpy(buf, bd->ptr, n);
    bd->ptr  += n;
    bd->size -= n;

    return static_cast<int>(n);
}

int
sws_colorspace_for(AVColorSpace space, int height)
{
    switch (space) {
    case AVCOL_SPC_BT709:      return SWS_CS_ITU709;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:  return SWS_CS_ITU601;
    case AVCOL_SPC_SMPTE240M:  return SWS_CS_SMPTE240M;
    case AVCOL_SPC_BT2020_NCL: return SWS_CS_BT2020;
    default:                   return height >= 720 ? SWS_CS_ITU709 : SWS_CS_ITU601;
    }
}

bool
is_full_range(AVColorRange range, AVPixelFormat fmt)
{
    if (range != AVCOL_RANGE_UNSPECIFIED)
        return range == AVCOL_RANGE_JPEG;

    return fmt == AV_PIX_FMT_YUVJ420P || fmt == AV_PIX_FMT_YUVJ422P ||
           fmt == AV_PIX_FMT_YUVJ444P || fmt == AV_PIX_FMT_YUVJ440P;
}

using format_ctx_ptr = std::unique_ptr<AVFormatContext, AVFormatCtxDeleter>;

// Opens a local file and probes its streams. Null on any failure.
format_ctx_ptr
open_local_media (const QString &local_path)
{
    if (local_path.isEmpty())
        return {};

    // Explicit "file:" so a name like "a:b.mkv" is never parsed as a protocol URL.
    const QByteArray url = "file:" + local_path.toUtf8();

    AVFormatContext *raw_fmt = nullptr;
    if (avformat_open_input(&raw_fmt, url.constData(), nullptr, nullptr) < 0)
        return {};                                   // FFmpeg already freed it

    format_ctx_ptr fmt_ctx(raw_fmt);

    if (avformat_find_stream_info(fmt_ctx.get(), nullptr) < 0)
        return {};

    return fmt_ctx;
}

// First real video stream, or nullptr. Embedded cover art shows up as a "video"
// stream flagged ATTACHED_PIC; that's TagLib's job (extract_cover), not ours.
// The result is owned by fmt_ctx and must not outlive it.
AVStream *
find_video_stream (const AVFormatContext *fmt_ctx)
{
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        AVStream *st = fmt_ctx->streams[i];
        const bool is_cover_art = (st->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0;

        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && !is_cover_art)
            return st;
    }

    return nullptr;
}

} // namespace

/* Runs sws_scale without ever letting swscale touch the QImage allocation.
   Output goes into an FFmpeg-owned, aligned, padded buffer, then is copied
   row by row into the QImage. */
QImage
sws_convert_to_qimage (const uint8_t *const *src_data, const int *src_linesize,
                             int src_w, int src_h, AVPixelFormat src_fmt,
                             int dst_w, int dst_h, int flags,
                             AVPixelFormat dst_fmt, QImage::Format out_format,
                             const source_colorimetry &colorimetry)
{
    if (src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
        return {};
    if (src_w > 16384 || src_h > 16384)      // corrupt-header guard
        return {};

    SwsContext *sws = sws_getContext(src_w, src_h, src_fmt,
                                     dst_w, dst_h, dst_fmt,
                                     flags, nullptr, nullptr, nullptr);
    if (!sws)
        return {};

    // The caller states what the source really is. The defaults reproduce the
    // old forced JPEG behavior (decoders don't always report yuvj* formats).
    sws_setColorspaceDetails(sws,
                             sws_getCoefficients(sws_colorspace_for(colorimetry.space, src_h)),
                             is_full_range(colorimetry.range, src_fmt) ? 1 : 0,
                             sws_getCoefficients(SWS_CS_ITU709), 0,
                             0, 1 << 16, 1 << 16);

    uint8_t *dst_data[4]     = {};
    int      dst_linesize[4] = {};

    if (av_image_alloc(dst_data, dst_linesize, dst_w, dst_h, dst_fmt, 32) < 0) {
        sws_freeContext(sws);
        return {};
    }

    sws_scale(sws, src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
    sws_freeContext(sws);

    QImage img(dst_w, dst_h, out_format);
    if (img.isNull()) {
        av_freep(&dst_data[0]);
        return {};
    }
    const size_t row = std::min<size_t>(img.bytesPerLine(), dst_linesize[0]);

    for (int y = 0; y < dst_h; ++y)
        std::memcpy(img.scanLine(y),
                    dst_data[0] + static_cast<ptrdiff_t>(y) * dst_linesize[0],
                    row);
    av_freep(&dst_data[0]);
    return img;
}

QImage
decode_cover_ffmpeg(const uchar *data, size_t size, QImage::Format out_format)
{
    if (!data || size == 0)
        return {};

    const AVPixelFormat dst_fmt = av_format_for_qimage(out_format);
    if (dst_fmt == AV_PIX_FMT_NONE)
        return {};   // never write a layout the QImage wasn't sized for

    BufferData bd{ reinterpret_cast<const uint8_t *>(data), size };

    constexpr int avio_ctx_buffer_size = 4096;
    uint8_t *avio_buffer = static_cast<uint8_t *>(av_malloc(avio_ctx_buffer_size));
    if (!avio_buffer)
        return {};

    std::unique_ptr<AVIOContext, AVIOCtxDeleter> avio_ctx(
        avio_alloc_context(avio_buffer, avio_ctx_buffer_size, 0,
                           &bd, read_packet, nullptr, nullptr));
    if (!avio_ctx) {
        av_free(avio_buffer);
        return {};
    }

    std::unique_ptr<AVFormatContext, AVFormatCtxDeleter> fmt_ctx(avformat_alloc_context());
    if (!fmt_ctx)
        return {};
    fmt_ctx->pb = avio_ctx.get();

    // avformat_open_input takes ownership and frees the context on failure.
    // Relinquish ownership explicitly beforehand.
    AVFormatContext *raw_fmt = fmt_ctx.release();
    
    if (avformat_open_input(&raw_fmt, nullptr, nullptr, nullptr) < 0) {
        if (raw_fmt) {                   // very old FFmpeg kept it allocated
            avformat_free_context(raw_fmt);
        }
        return {};
    }
    
    // Re-acquire ownership on success so AVFormatCtxDeleter manages it safely
    fmt_ctx.reset(raw_fmt);

    if (avformat_find_stream_info(fmt_ctx.get(), nullptr) < 0)
        return {};

    int video_stream_index = -1;
    const AVCodec *codec = nullptr;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = static_cast<int>(i);
            codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            break;
        }
    }
    if (video_stream_index < 0 || !codec)
        return {};

    std::unique_ptr<AVCodecContext, AVCodecCtxDeleter> codec_ctx(avcodec_alloc_context3(codec));
    if (!codec_ctx)
        return {};
    if (avcodec_parameters_to_context(codec_ctx.get(),
                                      fmt_ctx->streams[video_stream_index]->codecpar) < 0)
        return {};
    if (avcodec_open2(codec_ctx.get(), codec, nullptr) < 0)
        return {};

    std::unique_ptr<AVFrame,  AVFrameDeleter>  frame(av_frame_alloc());
    std::unique_ptr<AVPacket, AVPacketDeleter> pkt(av_packet_alloc());
    if (!frame || !pkt)
        return {};

    while (av_read_frame(fmt_ctx.get(), pkt.get()) >= 0) {
        QImage result;
        bool got_frame = false;

        if (pkt->stream_index == video_stream_index &&
            avcodec_send_packet(codec_ctx.get(), pkt.get()) == 0) {
            if (avcodec_receive_frame(codec_ctx.get(), frame.get()) == 0) {
                got_frame = true;
                if (frame->format != AV_PIX_FMT_NONE) {
                    // Ground truth is frame->format, not codec_ctx->pix_fmt.
                    result = sws_convert_to_qimage(frame->data, frame->linesize,
                                                   frame->width, frame->height,
                                                   static_cast<AVPixelFormat>(frame->format),
                                                   frame->width, frame->height, SWS_POINT,
                                                   dst_fmt, out_format);
                }
                av_frame_unref(frame.get());
            }
        }
        av_packet_unref(pkt.get());
        if (got_frame)
            return result;               // possibly null if conversion failed
    }

    return {};
}

bool
has_video_stream (const QString &local_path)
{
    const format_ctx_ptr fmt_ctx = open_local_media(local_path);

    return fmt_ctx && find_video_stream(fmt_ctx.get());
}

QImage
decode_video_frame_at_fraction (const QString &local_path, double fraction,
                                QImage::Format out_format)
{
    const AVPixelFormat dst_fmt = av_format_for_qimage(out_format);
    if (dst_fmt == AV_PIX_FMT_NONE)
        return {};

    // 1-2. Open, probe, and require a real video stream.
    format_ctx_ptr fmt_ctx = open_local_media(local_path);
    if (!fmt_ctx)
        return {};

    AVStream *stream = find_video_stream(fmt_ctx.get());
    if (!stream)
        return {};

    // Let the demuxer skip every other stream's packets.
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i)
        fmt_ctx->streams[i]->discard =
            (fmt_ctx->streams[i] == stream) ? AVDISCARD_DEFAULT : AVDISCARD_ALL;

    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
        return {};

    std::unique_ptr<AVCodecContext, AVCodecCtxDeleter> codec_ctx(avcodec_alloc_context3(codec));
    if (!codec_ctx ||
        avcodec_parameters_to_context(codec_ctx.get(), stream->codecpar) < 0 ||
        avcodec_open2(codec_ctx.get(), codec, nullptr) < 0)
        return {};

    std::unique_ptr<AVFrame,  AVFrameDeleter>  frame(av_frame_alloc());
    std::unique_ptr<AVPacket, AVPacketDeleter> pkt(av_packet_alloc());
    if (!frame || !pkt)
        return {};

    // 3. Target position. Prefer the stream's own duration, fall back to the
    //    container's. AV_TIME_BASE_Q is a C compound literal, so build our own.
    constexpr AVRational time_base_q { 1, AV_TIME_BASE };

    int64_t duration = stream->duration;
    if (duration <= 0)
        duration = fmt_ctx->duration > 0
                 ? av_rescale_q(fmt_ctx->duration, time_base_q, stream->time_base)
                 : 0;

    // Timestamps don't always start at 0 (MPEG-TS commonly doesn't).
    int64_t start = stream->start_time;
    if (start == AV_NOPTS_VALUE)
        start = fmt_ctx->start_time != AV_NOPTS_VALUE
              ? av_rescale_q(fmt_ctx->start_time, time_base_q, stream->time_base)
              : 0;

    const int64_t offset = static_cast<int64_t>(
        static_cast<double>(duration) * std::clamp(fraction, 0.0, 1.0));

    // 4. BACKWARD = land on the keyframe at or before the target (GOP start).
    //    A failed seek is non-fatal: we just start from the beginning.
    if (offset > 0) {
        av_seek_frame(fmt_ctx.get(), stream->index, start + offset, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codec_ctx.get());
    }

    // 5. Decode exactly one picture. Only keyframe packets are ever fed to the
    //    decoder, and each is followed by an immediate drain: otherwise a
    //    decoder with reorder delay would sit on the I-frame until it saw
    //    later packets, forcing us to decode (or at least read) a whole GOP.
    //    Skipping non-keyframes also avoids open-GOP leading frames whose
    //    references are missing after a seek.
    constexpr int max_packets = 512;   // bounded work on broken/keyframe-less files

    for (int n = 0; n < max_packets && av_read_frame(fmt_ctx.get(), pkt.get()) >= 0; ++n) {
        const bool is_keyframe = pkt->stream_index == stream->index &&
                                 (pkt->flags & AV_PKT_FLAG_KEY);
        bool got_frame = false;

        if (is_keyframe) {
            got_frame = avcodec_send_packet(codec_ctx.get(), pkt.get()) == 0
                     && avcodec_send_packet(codec_ctx.get(), nullptr) == 0
                     && avcodec_receive_frame(codec_ctx.get(), frame.get()) == 0;
        }
        av_packet_unref(pkt.get());

        if (got_frame) {
            if (frame->format == AV_PIX_FMT_NONE)
                return {};

            return sws_convert_to_qimage(frame->data, frame->linesize,
                                         frame->width, frame->height,
                                         static_cast<AVPixelFormat>(frame->format),
                                         frame->width, frame->height, SWS_BILINEAR,
                                         dst_fmt, out_format,
                                         { frame->colorspace, frame->color_range });
        }

        if (is_keyframe)
            avcodec_flush_buffers(codec_ctx.get());   // leave draining state, try next keyframe
    }

    return {};
}


} // namespace covers::decode