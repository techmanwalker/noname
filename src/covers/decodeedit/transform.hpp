#pragma once

#include <QImage>
#include <cstddef>

namespace covers::transform
{

using ratio = std::pair<size_t, size_t>;

[[nodiscard]] constexpr ratio
find_aspect_ratio(size_t width, size_t height) noexcept
{
    const size_t g = std::gcd(width, height);

    // gcd(0, 0) == 0 would divide by zero. A degenerate 0x0 surface has no
    // meaningful ratio, so it passes through unchanged.
    if (g == 0)
        return { 0, 0 };

    return { width / g, height / g };
}

QSizeF largest_aspect_size(const QSizeF &bounds,
                           double aspect_w,
                           double aspect_h);

QImage crop_largest_aspect(const QImage &image,
                           double aspect_w = 1.0,
                           double aspect_h = 1.0);

QImage lanczos_resize(const QImage &image, size_t width, size_t height);

// Size to scale `source` to for a request that may leave a side open (0).
// Both set: as requested. One 0: that side follows source's aspect ratio. Both 0: source size.
[[nodiscard]] QSize scaled_size (QSize source, QSize requested) noexcept;


}
