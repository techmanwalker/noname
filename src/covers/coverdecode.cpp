#include "coverdecode.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
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

/* Runs sws_scale without ever letting swscale touch the QImage allocation.
   Output goes into an FFmpeg-owned, aligned, padded buffer, then is copied
   row by row into the QImage. */
QImage
sws_convert_to_qimage (const uint8_t *const *src_data, const int *src_linesize,
                             int src_w, int src_h, AVPixelFormat src_fmt,
                             int dst_w, int dst_h, int flags,
                             AVPixelFormat dst_fmt, QImage::Format out_format)
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

    // JPEG/MJPEG is full-range YUV; the decoder doesn't always report the
    // yuvj* formats, so force the range to avoid washed-out colors.
    sws_setColorspaceDetails(sws,
                             sws_getCoefficients(SWS_CS_ITU601), 1,
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

} // namespace

QImage
lanczos_resize_square(const QImage &image, int target_size)
{
    const QImage src = image.convertToFormat(pixelformat_qimage);
    
    // Calculate the largest possible centered square
    const int crop_size = std::min(src.width(), src.height());
    const int crop_x = (src.width() - crop_size) / 2;
    const int crop_y = (src.height() - crop_size) / 2;
    
    // Find the exact byte where the crop region starts
    const int bytes_per_pixel = src.depth() / 8;
    const uint8_t *src_slices[1] = { 
        src.constScanLine(crop_y) + (crop_x * bytes_per_pixel) 
    };
    
    // Keep the original QImage stride so swscale correctly skips the margins
    int src_strides[1] = { static_cast<int>(src.bytesPerLine()) };

    // Feed crop_size as the input dimensions
    return sws_convert_to_qimage(src_slices, src_strides,
                                 crop_size, crop_size, av_format_for_qimage(pixelformat_qimage),
                                 target_size, target_size, SWS_LANCZOS,
                                 av_format_for_qimage(pixelformat_qimage), pixelformat_qimage);
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

} // namespace covers::decode