#pragma once

#include <QImage>

namespace covers::transform
{

QSizeF largest_aspect_size(const QSizeF &bounds,
                           double aspect_w,
                           double aspect_h);
    
QImage crop_largest_aspect(const QImage &image,
                           double aspect_w = 1.0,
                           double aspect_h = 1.0);

QImage lanczos_resize(const QImage &image, size_t width, size_t height);

QImage lanczos_resize_square(const QImage &image, size_t target_size);


}