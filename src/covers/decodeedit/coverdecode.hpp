#pragma once

#include <QImage>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace covers::decode {

QImage
sws_convert_to_qimage (const uint8_t *const *src_data, const int *src_linesize,
                             int src_w, int src_h, AVPixelFormat src_fmt,
                             int dst_w, int dst_h, int flags,
                             AVPixelFormat dst_fmt, QImage::Format out_format);

QImage decode_cover_ffmpeg(const uchar *data, size_t size, QImage::Format out_format);

}