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

// Percentile of OkLab lightness (L, in [0,1]) sampled from a *ring* or *cell*
// of `cover`. Deliberately not the mean, for the same reason as always — the
// mean gets dragged around by large flat regions, which this exists to
// avoid. percentile is 0-100. ring_crop_begin/ring_crop_end (each 0-1)
// bound the ring by Chebyshev distance from the image's own center: 0 is
// dead center, 1 is the image's own edge (corners included, since this is
// a rectangular frame, not a circular one). [0, 1] (the defaults) samples
// the whole image; [0, 0.6] samples a solid center block; [0.8, 1] samples
// only the outer border frame. Adjacent rings never share a pixel — the
// interval is half-open on the high end ([begin, end)) except at the true
// outer edge, where end == 1 includes the literal edge pixels too.
ring_luma percentile_luminance (const QImage &chunk, int percentile,
                                double ring_crop_begin = 0.0, double ring_crop_end = 1.0);

// Area-weighted mean of the four ring lumas in `lumas`, condensed to a
// single [0,1] scalar -- the input the future dark-keyed/light-keyed
// background curves will read, replacing a single percentile_luminance
// reading. Each ring's weight is the fraction of the image's area it
// covers: exactly ring_crop_end^2 - ring_crop_begin^2, regardless of the
// cover's aspect ratio. That's not an approximation -- percentile_luminance
// normalizes each axis independently before taking the Chebyshev max, so
// the region within normalized distance s of center is always the whole
// image rectangle scaled by s about its own center, on both axes at once;
// its area is exactly s^2 of the total, no width/height needed.
// Rings that tile [0,1] edge-to-edge sum to exactly 1, making the
// normalization below a no-op -- it stays regardless, so a gap, overlap,
// or malformed ring (begin > end) degrades gracefully instead of silently
// skewing the result. Falls back to 0.5 -- the same "nothing computed
// yet" placeholder used elsewhere -- only if every ring somehow
// contributes zero area at once.
double pondered_luma (const cover_rings &lumas);

// Divides `image` into s_rows x s_columns cells and stores the percentile
// luminance of each one. Cell edges are rounded, but every pixel belongs to
// at least one cell. A null image yields an all-zero table with ratio {0, 0}.
table_luma luma_table_of_image (const QImage &image, int percentile);

}