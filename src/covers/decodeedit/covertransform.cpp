#include "coverdecode.hpp"
#include "covertransform.hpp"
#include "pixelformats.hpp"
#include <libswscale/swscale.h>

namespace covers::transform
{

static_assert(find_aspect_ratio(1920, 1080) == ratio{ 16, 9 });
static_assert(find_aspect_ratio(1919, 1080) == ratio{ 1919, 1080 });
static_assert(find_aspect_ratio(640, 480)   == ratio{ 4, 3 });
static_assert(find_aspect_ratio(720, 960)   == ratio{ 3, 4 });
static_assert(find_aspect_ratio(1366, 768)  == ratio{ 683, 384 });
static_assert(find_aspect_ratio(0, 0)       == ratio{ 0, 0 });

// Dimensions of the largest rectangle with aspect ratio aspect_w:aspect_h
// that fits entirely inside `bounds` (one side matches `bounds`, the other
// is smaller or equal). Only computes a size: the caller decides where to
// place it (e.g. centered, for a crop). Exact (no rounding); returns an
// empty size on invalid input.
QSizeF
largest_aspect_size(const QSizeF &bounds, double aspect_w, double aspect_h)
{
    if (bounds.isEmpty() ||
        !std::isfinite(aspect_w) || !std::isfinite(aspect_h) ||
        aspect_w <= 0.0 || aspect_h <= 0.0)
        return {};

    // (bw / bh) > (aw / ah)  <=>  bw * ah > bh * aw
    if (bounds.width() * aspect_h > bounds.height() * aspect_w)
        return { bounds.height() * aspect_w / aspect_h, bounds.height() };

    return { bounds.width(), bounds.width() * aspect_h / aspect_w };
}

QImage
crop_largest_aspect(const QImage &image, double aspect_w, double aspect_h)
{
    if (image.isNull())
        return {};

    const QSizeF fit = largest_aspect_size(image.size(), aspect_w, aspect_h);
    if (fit.isEmpty())
        return {};

    const int w = image.width();
    const int h = image.height();

    const int crop_w = std::clamp(static_cast<int>(std::lround(fit.width())),  1, w);
    const int crop_h = std::clamp(static_cast<int>(std::lround(fit.height())), 1, h);

    if (crop_w == w && crop_h == h) {
        return image; // it already has the requested aspect ratio
    }

    const QImage src = image.convertToFormat(pixelformat_qimage);

    const int crop_x = (w - crop_w) / 2;
    const int crop_y = (h - crop_h) / 2;

    return src.copy(crop_x, crop_y, crop_w, crop_h);
}

QImage
lanczos_resize(const QImage &image, size_t width, size_t height)
{
    if (image.isNull()) {
        return {};
    }
    
    if (width == 0 || height == 0) {
        return image; // unspecified size = full res
    }

    const QImage src = image.convertToFormat(pixelformat_qimage);
    
    const uint8_t *src_slices[1] = { src.constScanLine(0) };
    int src_strides[1] = { static_cast<int>(src.bytesPerLine()) };

    return decode::sws_convert_to_qimage(src_slices, src_strides,
                                 src.width(), src.height(), av_format_for_qimage(pixelformat_qimage),
                                 static_cast<int>(width), static_cast<int>(height), SWS_LANCZOS,
                                 av_format_for_qimage(pixelformat_qimage), pixelformat_qimage);
}

QImage
lanczos_resize_square(const QImage &image, size_t target_size)
{
    const QImage cropped = crop_largest_aspect(image);
    return lanczos_resize(cropped, static_cast<size_t>(target_size), static_cast<size_t>(target_size));
}

}