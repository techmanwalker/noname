#include "decodeedit/codecs.hpp"
#include "decodeedit/pixelformats.hpp"

#include "logging/l_coverprovider.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
}

#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/decode.h>
#include <jxl/decode_cxx.h>
#include <jxl/resizable_parallel_runner_cxx.h>

#include <QFile>
#include <QImage>

#include <algorithm>
#include <cstring>
#include <memory>

namespace covers::codecs {

struct BufferData {
    const uint8_t *ptr;
    size_t size;
};

namespace {

struct AVIOCtxDeleter {
    void operator()(AVIOContext *ctx) const {
        if (ctx) { av_free(ctx->buffer); avio_context_free(&ctx); }
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

// FFmpeg exposes flag fields as plain int; test them as unsigned so the
// bitwise operation never touches a signed operand.
constexpr bool
has_flag (int value, int flag)
{
    return (static_cast<unsigned>(value) & static_cast<unsigned>(flag)) != 0;
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
        const bool is_cover_art = has_flag(st->disposition, AV_DISPOSITION_ATTACHED_PIC);

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

    constexpr int fixed_one = 0x10000;   // 1.0 in swscale's 16.16 fixed point

    // The caller states what the source really is. The defaults reproduce the
    // old forced JPEG behavior (decoders don't always report yuvj* formats).
    sws_setColorspaceDetails(sws,
                             sws_getCoefficients(sws_colorspace_for(colorimetry.space, src_h)),
                             is_full_range(colorimetry.range, src_fmt) ? 1 : 0,
                             sws_getCoefficients(SWS_CS_ITU709), 0,
                             0, fixed_one, fixed_one);   // brightness, contrast, saturation

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
        av_free(dst_data[0]);
        return {};
    }
    const size_t row = std::min<size_t>(img.bytesPerLine(), dst_linesize[0]);

    for (int y = 0; y < dst_h; ++y)
        std::memcpy(img.scanLine(y),
                    dst_data[0] + static_cast<ptrdiff_t>(y) * dst_linesize[0],
                                        row);
    av_free(dst_data[0]);
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
                                 has_flag(pkt->flags, AV_PKT_FLAG_KEY);
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

QImage
read_jpegxl(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        /*
        qCWarning(l_coverprovider) << "failed to open thumbnail" << path
                                << file.errorString();
                                */
        return QImage();
    }

    const QByteArray compressed = file.readAll();
    if (compressed.isEmpty()) {
        qCWarning(l_coverprovider) << "empty JPEG XL file" << path;
        return QImage();
    }

    auto dec = JxlDecoderMake(nullptr);
    if (!dec) {
        qCWarning(l_coverprovider) << "failed to create JxlDecoder for" << path;
        return QImage();
    }

    auto runner = JxlResizableParallelRunnerMake(nullptr);
    if (JXL_DEC_SUCCESS != JxlDecoderSetParallelRunner(dec.get(),
            JxlResizableParallelRunner, runner.get()))
    {
        qCWarning(l_coverprovider) << "failed to set decoder parallel runner" << path;
        return QImage();
    }

    if (JXL_DEC_SUCCESS != JxlDecoderSubscribeEvents(dec.get(),
            JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE))
    {
        qCWarning(l_coverprovider) << "JxlDecoderSubscribeEvents failed" << path;
        return QImage();
    }

    JxlDecoderSetInput(dec.get(),
                        reinterpret_cast<const uint8_t*>(compressed.constData()),
                        size_t(compressed.size()));
    JxlDecoderCloseInput(dec.get());

    JxlBasicInfo info;
    JxlPixelFormat format = {4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};

    std::vector<uint8_t> pixels;
    bool got_image = false;

    for (;;) {
        JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

        if (status == JXL_DEC_ERROR) {
            qCWarning(l_coverprovider) << "JPEG XL decoder error" << path;
            return QImage();
        }
        if (status == JXL_DEC_NEED_MORE_INPUT) {
            qCWarning(l_coverprovider) << "JPEG XL needs more input (truncated?)" << path;
            return QImage();
        }
        if (status == JXL_DEC_BASIC_INFO) {
            if (JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo(dec.get(), &info)) {
                qCWarning(l_coverprovider) << "JxlDecoderGetBasicInfo failed" << path;
                return QImage();
            }
            if (info.xsize == 0 || info.ysize == 0) {
                qCWarning(l_coverprovider) << "invalid dimensions in" << path;
                return QImage();
            }
        }
        else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            size_t buffer_size = 0;
            if (JXL_DEC_SUCCESS != JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size)) {
                qCWarning(l_coverprovider) << "JxlDecoderImageOutBufferSize failed" << path;
                return QImage();
            }

            pixels.resize(buffer_size);
            if (JXL_DEC_SUCCESS != JxlDecoderSetImageOutBuffer(dec.get(), &format,
                                                                pixels.data(), buffer_size))
            {
                qCWarning(l_coverprovider) << "JxlDecoderSetImageOutBuffer failed" << path;
                return QImage();
            }
        }
        else if (status == JXL_DEC_FULL_IMAGE) {
            got_image = true;
        }
        else if (status == JXL_DEC_SUCCESS) {
            break;
        }
    }

    if (!got_image || pixels.empty()) {
        qCWarning(l_coverprovider) << "no image decoded from" << path;
        return QImage();
    }

    // Force deep copy
    QImage image(pixels.data(),
                    int(info.xsize), int(info.ysize),
                    qsizetype(info.xsize) * 4,
                    pixelformat_qimage);

    return image.copy();
}

bool
write_jpegxl(const QString &path, QImage image, QImage::Format format)
{
    if (image.isNull()) {
        qCWarning(l_coverprovider) << "refusing to write a null thumbnail for " << path;
        return false;
    }

    const QImage converted_buffer = image.format() == format
        ? image
        : image.convertToFormat(format);

    const int width  = converted_buffer.width();
    const int height = converted_buffer.height();
    const size_t pixels = size_t(width) * size_t(height);
    const size_t bytes  = pixels * 4;

    // Tightly packed RGBA8888 buffer
    const size_t row_bytes = size_t(width) * 4;
    std::vector<uint8_t> packed(bytes);
    for (int y = 0; y < height; ++y) {
        std::memcpy(packed.data() + size_t(y) * row_bytes,
                    converted_buffer.constScanLine(y),
                    row_bytes);
    }

    auto enc = JxlEncoderMake(nullptr);
    if (!enc) {
        qCWarning(l_coverprovider) << "failed to create JxlEncoder for" << path;
        return false;
    }

    auto runner = JxlResizableParallelRunnerMake(nullptr);
    if (JXL_ENC_SUCCESS != JxlEncoderSetParallelRunner(enc.get(),
            JxlResizableParallelRunner, runner.get()))
    {
        qCWarning(l_coverprovider) << "failed to set parallel runner for" << path;
        return false;
    }

    JxlBasicInfo basic_info;
    JxlEncoderInitBasicInfo(&basic_info);
    basic_info.xsize = width;
    basic_info.ysize = height;
    basic_info.bits_per_sample = 8;
    basic_info.alpha_bits = 8;
    basic_info.num_color_channels = 3;
    basic_info.num_extra_channels = 1;          // alpha
    basic_info.uses_original_profile = JXL_FALSE;

    if (JXL_ENC_SUCCESS != JxlEncoderSetBasicInfo(enc.get(), &basic_info)) {
        qCWarning(l_coverprovider) << "JxlEncoderSetBasicInfo failed for" << path;
        return false;
    }

    JxlColorEncoding color_encoding;
    JxlColorEncodingSetToSRGB(&color_encoding, /*is_gray=*/JXL_FALSE);
    if (JXL_ENC_SUCCESS != JxlEncoderSetColorEncoding(enc.get(), &color_encoding)) {
        qCWarning(l_coverprovider) << "JxlEncoderSetColorEncoding failed for" << path;
        return false;
    }

    JxlEncoderFrameSettings *frame_settings = JxlEncoderFrameSettingsCreate(enc.get(), nullptr);

    // Requested settings
    JxlEncoderSetFrameDistance(frame_settings, 1.0f);                 // distance = 1
    JxlEncoderFrameSettingsSetOption(frame_settings,
                                     JXL_ENC_FRAME_SETTING_EFFORT, 7); // effort = 7

    JxlPixelFormat pixel_format = {
        4,                      // num_channels (RGBA)
        JXL_TYPE_UINT8,
        JXL_NATIVE_ENDIAN,
        0                       // align
    };

    if (JXL_ENC_SUCCESS != JxlEncoderAddImageFrame(frame_settings,
                                                   &pixel_format,
                                                   packed.data(),
                                                   bytes))
    {
        qCWarning(l_coverprovider) << "JxlEncoderAddImageFrame failed for" << path;
        return false;
    }

    JxlEncoderCloseInput(enc.get());

    std::vector<uint8_t> compressed;
    compressed.resize(size_t{64} * 1024);   // start with 64 KiB, grow as needed

    uint8_t *next_out = compressed.data();
    size_t avail_out = compressed.size();

    JxlEncoderStatus status = JXL_ENC_NEED_MORE_OUTPUT;
    while (status == JXL_ENC_NEED_MORE_OUTPUT) {
        status = JxlEncoderProcessOutput(enc.get(), &next_out, &avail_out);

        if (status == JXL_ENC_NEED_MORE_OUTPUT) {
            size_t offset = next_out - compressed.data();
            compressed.resize(compressed.size() * 2);
            next_out = compressed.data() + offset;
            avail_out = compressed.size() - offset;
        }
    }

    if (status != JXL_ENC_SUCCESS) {
        qCWarning(l_coverprovider) << "JxlEncoderProcessOutput failed for" << path;
        return false;
    }

    compressed.resize(next_out - compressed.data());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(l_coverprovider) << "failed to open thumbnail for writing" << path
                                << file.errorString();
        return false;
    }

    if (file.write(reinterpret_cast<const char*>(compressed.data()),
                   qint64(compressed.size())) != qint64(compressed.size()))
    {
        qCWarning(l_coverprovider) << "failed to write JPEG XL data" << path
                                << file.errorString();
        file.remove();
        return false;
    }

    return true;
}

} // namespace covers::codecs
