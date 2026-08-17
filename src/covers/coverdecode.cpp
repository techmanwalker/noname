#include "coverdecode.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <QImage>

#include "pixelformats.hpp"

namespace covers::decode {

/* `square` must already be width == height. Returns a null QImage on
   swscale context failure (e.g. unsupported CPU path — practically never
   happens, but don't crash on it). */
QImage
lanczos_resize_square(const QImage &square, int target_size)
{
    const QImage src = square.convertToFormat(pixelformat_qimage);

    // qCDebug (song_factory::l_songfactory()) << "Attempting to rescale a cover. Source size: " << src.width() << "x" << src.height();

    SwsContext *sws = sws_getContext(
        src.width(), src.height(), pixelformat_ffmpeg,
        target_size, target_size, pixelformat_ffmpeg,
        SWS_LANCZOS, nullptr, nullptr, nullptr);
    if (!sws) {
        return QImage();
    }

    QImage dst(target_size, target_size, pixelformat_qimage);

    const uint8_t *src_slices[1] = { src.constBits() };
    int src_strides[1] = { static_cast<int>(src.bytesPerLine()) };
    uint8_t *dst_slices[1] = { dst.bits() };
    int dst_strides[1] = { static_cast<int>(dst.bytesPerLine()) };

    sws_scale(sws, src_slices, src_strides, 0, src.height(), dst_slices, dst_strides);
    sws_freeContext(sws);

    /* qCDebug (song_factory::l_songfactory()) << "Rescaling completed. Source size: " << src.width() << "x" << src.height()
        << "; destination size: " << dst.width() << "x" << dst.height();*/

    return dst;
}

// FFmpeg decoding covers

struct BufferData {
    const uint8_t *ptr;
    size_t size;
};

// Custom IO callback for FFmpeg to read from the TagLib::ByteVector memory
int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    BufferData *bd = static_cast<BufferData *>(opaque);
    buf_size = std::min(static_cast<size_t>(buf_size), bd->size);

    if (buf_size <= 0) {
        return AVERROR_EOF;
    }

    std::memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;

    return buf_size;
}

QImage decode_cover_ffmpeg(const uchar *data, size_t size, QImage::Format out_format)
{
    if (!data || size == 0) {
        return {};
    }

    // Cast the uchar pointer directly for FFmpeg's consumption
    BufferData bd = { reinterpret_cast<const uint8_t*>(data), size };

    constexpr int avio_ctx_buffer_size = 4096;
    uint8_t *avio_buffer = static_cast<uint8_t*>(av_malloc(avio_ctx_buffer_size));
    if (!avio_buffer) {
        return {};
    }

    AVIOContext *avio_ctx = avio_alloc_context(avio_buffer, avio_ctx_buffer_size, 0, &bd, &read_packet, nullptr, nullptr);
    if (!avio_ctx) {
        av_freep(&avio_buffer);
        return {};
    }

    AVFormatContext *fmt_ctx = avformat_alloc_context();
    fmt_ctx->pb = avio_ctx;

    // Open the memory buffer as a media file
    if (avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr) < 0) {
        av_freep(&avio_ctx->buffer);
        avio_context_free(&avio_ctx);
        avformat_free_context(fmt_ctx);
        return {};
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        if (avio_ctx) {
            av_freep(&avio_ctx->buffer);
            avio_context_free(&avio_ctx);
        }
        return {};
    }

    int video_stream_index = -1;
    const AVCodec *codec = nullptr;
    
    // Locate the first available video stream (the cover art)
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = static_cast<int>(i);
            codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            break;
        }
    }

    if (video_stream_index == -1 || !codec) {
        avformat_close_input(&fmt_ctx);
        if (avio_ctx) {
            av_freep(&avio_ctx->buffer);
            avio_context_free(&avio_ctx);
        }
        return {};
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        if (avio_ctx) {
            av_freep(&avio_ctx->buffer);
            avio_context_free(&avio_ctx);
        }
        return {};
    }

    AVFrame *frame = av_frame_alloc();
    AVPacket *pkt = av_packet_alloc();
    QImage result;

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                if (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    
                    // Allocate the Qt image buffer
                    result = QImage(frame->width, frame->height, out_format);
                    
                    // Use SWS_POINT for pure colorspace conversion without scaling overhead
                    SwsContext *sws = sws_getContext(
                        frame->width, frame->height, codec_ctx->pix_fmt,
                        frame->width, frame->height, pixelformat_ffmpeg, // Unifies to AV_PIX_FMT_RGBA[cite: 16]
                        SWS_POINT, nullptr, nullptr, nullptr
                    );

                    if (sws) {
                        uint8_t *dest_slices[1] = { result.bits() };
                        int dest_strides[1] = { static_cast<int>(result.bytesPerLine()) };
                        
                        // Map the raw decoded frame directly into the QImage memory
                        sws_scale(sws, frame->data, frame->linesize, 0, frame->height, dest_slices, dest_strides);
                        sws_freeContext(sws);
                    } else {
                        result = QImage();
                    }
                    
                    av_packet_unref(pkt);
                    break; // The cover is extracted, stop reading further frames
                }
            }
        }
        av_packet_unref(pkt);
    }

    // Cleanup resources
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    
    avformat_close_input(&fmt_ctx);
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        avio_context_free(&avio_ctx);
    }

    return result;
}

} // namespace