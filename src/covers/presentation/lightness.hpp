#pragma once

#include "decodeedit/transform.hpp"

#include <QRect>
#include <QSize>

#include <array>
#include <cstddef>
#include <cstdint>

class QImage;

namespace covers::lightness
{

constexpr double default_lightness_value = 0.5;

struct ring_lightness {
    double value = default_lightness_value;
    double ring_crop_begin = 0.0;
    double ring_crop_end = 1.0;
};

// to identify which ring is each value of
enum class lightness_types : uint8_t {
    nuclear,
    centric,
    midring,
    borders,
    count /* sentinel */
};

// one ring_lightness per lightness_types, always accessed through ring_of()
using cover_rings = std::array<ring_lightness, static_cast<size_t>(lightness_types::count)>;

[[nodiscard]] constexpr ring_lightness &
ring_of (cover_rings &rings, lightness_types type) noexcept
{
    return rings[static_cast<size_t>(type)];
}

[[nodiscard]] constexpr const ring_lightness &
ring_of (const cover_rings &rings, lightness_types type) noexcept
{
    return rings[static_cast<size_t>(type)];
}

/*  NxM table + aspect ratio representing equally sized chunks of an image
    (cover), each value corresponds to its lightness at a given percentile.

    [N][M] = [row][column]
*/
struct lightness_table {
    // The backdrop is shown blurred, so a glyph sees a local mean. Cells finer
    // than the blur are wasted (the old 512² was sub-pixel for a 256px thumbnail).
    static constexpr size_t s_rows = 128, s_columns = 128;

    // summed-area table of the blurred lightness: m_sat[r][c] = sum of cells [0,r) x [0,c)
    std::array<std::array<double, s_columns + 1>, s_rows + 1> m_sat {};

    transform::ratio m_aspect_ratio; // of the original image buffer

    [[nodiscard]] double backdrop_lightness_for_rect (QRect rect, QSize reference_surface) const;

    // blurred lightness of one cell, recovered from the summed-area table (debug dumps)
    [[nodiscard]] double cell (size_t row, size_t column) const noexcept
    {
        return m_sat[row + 1][column + 1] - m_sat[row][column + 1]
             - m_sat[row + 1][column]     + m_sat[row][column];
    }
};



double srgb_to_linear(uint8_t channel_8bit);

double oklab_lightness(double r_linear, double g_linear, double b_linear);

/*  Out of this image, crop a ring with bounds expressed in Chebyshev distance [0, 1]
    respect to the center and ask what lightness (L) score the nth percentile of pixels
    */
ring_lightness percentile_lightness (const QImage &chunk, int percentile,
                                double ring_crop_begin = 0.0, double ring_crop_end = 1.0);

/*  Based on the set of rings of a cover, calculate their weights over
    the entire expressed buffer [0, 1] and return its overall lightness.
    The returned magnitude is the overall value of L.
    */
double pondered_lightness (const cover_rings &rings);

/*  Divides `image` into s_rows x s_columns cells, takes the mean lightness of each,
    blurs the grid like the backdrop is blurred on screen, and stores it as a
    summed-area table. A null image yields an all-zero table with ratio {0, 0}.
    */
lightness_table lightness_table_of_image (const QImage &image);

}
