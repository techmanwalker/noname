#pragma once 

#include "covertransform.hpp"

#include <QRect>
#include <QSize>

#include <array>
#include <cstddef>
#include <cstdint>

class QImage;

namespace covers::luminance 
{

struct ring_luma {
    double value = 0.5;
    double ring_crop_begin = 0.0;
    double ring_crop_end = 1.0;
};

// to identify which ring is each value of
enum class luma_types : uint8_t {
    nuclear,
    centric,
    midring,
    borders,
    count /* sentinel */
};

// one ring_luma per luma_types, always accessed through ring_of()
using cover_rings = std::array<ring_luma, static_cast<size_t>(luma_types::count)>;

[[nodiscard]] constexpr ring_luma &
ring_of (cover_rings &rings, luma_types type) noexcept
{
    return rings[static_cast<size_t>(type)];
}

[[nodiscard]] constexpr const ring_luma &
ring_of (const cover_rings &rings, luma_types type) noexcept
{
    return rings[static_cast<size_t>(type)];
}

/*  NxM table + aspect ratio representing equally sized chunks of an image
    (cover), each value corresponds to its luma at a given percentile.

    [N][M] = [row][column]
*/
struct table_luma {
    // NxM size
    static constexpr size_t s_rows = 512, s_columns = 512;

    // percentile luminance of each equally sized region
    std::array<
        std::array <
            double,
            s_columns
        >,
        s_rows
    > m_table {};

    transform::ratio m_aspect_ratio; // of the original image buffer

    // use the table luminance values to find an average or percentile
    // luminance behind rect, given that it is located respect to the
    // corners of reference_surface.
    [[nodiscard]] double backdrop_luma_for_rect (QRect rect, QSize reference_surface,
                                                 int percentile = 60) const;
};

double srgb_to_linear(uint8_t channel_8bit);

double oklab_lightness(double r_linear, double g_linear, double b_linear);

/*  Out of this image, crop a ring with bounds expressed in Chebyshev distance [0, 1]
    respect to the center and ask what luma (L) score the nth percentile of pixels
    */
ring_luma percentile_luminance (const QImage &chunk, int percentile,
                                double ring_crop_begin = 0.0, double ring_crop_end = 1.0);

/*  Based on the set of rings of a cover, calculate their weights over
    the entire expressed buffer [0, 1] and return its overall luminance.
    The returned magnitude is the overall value of L. 
    */
double pondered_luma (const cover_rings &lumas);

/*  Which ring covers the largest part of `rect`; rect is relative to the
    top-left corner of `reference_surface`, nullopt if no ring covers a
    visible part of the rect.
    */
[[nodiscard]] std::optional<luma_types>
dominant_ring_for_rect (const cover_rings &rings, transform::ratio cover_aspect,
                        QRect rect, QSize reference_surface);

/*  Divides `image` into s_rows x s_columns cells and stores the percentile
    luminance of each one. Cell edges are rounded, but every pixel belongs to
    at least one cell. A null image yields an all-zero table with ratio {0, 0}.
    */
table_luma luma_table_of_image (const QImage &image, int percentile);

}