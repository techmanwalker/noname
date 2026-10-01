#pragma once

#include <QImage>

namespace covers::decode {

QSizeF largest_aspect_size(const QSizeF &bounds,
                           double aspect_w,
                           double aspect_h);
    
QImage crop_largest_aspect(const QImage &image,
                           double aspect_w = 1.0,
                           double aspect_h = 1.0);

QImage lanczos_resize(const QImage &image, size_t width, size_t height);

QImage lanczos_resize_square(const QImage &image, size_t target_size);

QImage decode_cover_ffmpeg(const uchar *data, size_t size, QImage::Format out_format);

double srgb_to_linear(uint8_t channel_8bit);

double oklab_lightness(double r_linear, double g_linear, double b_linear);

}