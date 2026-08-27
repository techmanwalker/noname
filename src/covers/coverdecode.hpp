#pragma once

#include <QImage>

namespace covers::decode {

QImage crop_largest_square(const QImage &image);

QImage lanczos_resize(const QImage &image, size_t width, size_t height);

QImage lanczos_resize_square(const QImage &image, size_t target_size);

QImage decode_cover_ffmpeg(const uchar *data, size_t size, QImage::Format out_format);

}