#pragma once

#include <QImage>

namespace covers::decode {

QImage lanczos_resize_square(const QImage &image, int target_size);

int read_packet(void *opaque, uint8_t *buf, int buf_size);

QImage decode_cover_ffmpeg(const uchar *data, size_t size, QImage::Format out_format);

}