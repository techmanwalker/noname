#include "presentation/lightness.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <memory>
#include <numeric>
#include <vector>

#include <QImage>
#include <QtConcurrent/QtConcurrent>

namespace covers::lightness
{

namespace {

struct srgb_tables {
    std::array<double, 256> linear_d{};
    std::array<float, 256>  linear_f{};

    srgb_tables () noexcept
    {
        for (size_t i = 0; i < 256; ++i) {
            const double c = static_cast<double>(i) / 255.0;
            const double v = c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
            linear_d[i] = v;
            linear_f[i] = static_cast<float>(v);
        }
    }
};

// safe to call from other static initializers and from any worker thread
[[nodiscard]] const srgb_tables &
srgb ()
{
    static const srgb_tables tables;
    return tables;
}

namespace oklab
{

// M1: linear sRGB -> LMS cone responses. Rows are L, M, S; columns are R, G, B.
template <typename T>
inline constexpr std::array<std::array<T, 3>, 3> lms_from_linear_srgb {{
    { T(0.4122214708), T(0.5363325363), T(0.0514459929) },
    { T(0.2119034982), T(0.6806995451), T(0.1073969566) },
    { T(0.0883024619), T(0.2817188376), T(0.6299787005) },
}};

// First row of M2: cube-rooted LMS -> OkLab L. The a and b rows are not needed.
template <typename T>
inline constexpr std::array<T, 3> lightness_from_lms_cbrt {
    T(0.2104542553), T(0.7936177850), T(-0.0040720468)
};

template <typename T>
[[nodiscard]] constexpr T
dot3 (const std::array<T, 3> &k, T x, T y, T z) noexcept
{
    return k[0] * x + k[1] * y + k[2] * z;
}

// Floor for an LMS response before the cube root: keeps cbrt_halley away from 0,
// where y^3 goes subnormal and 0/0 becomes NaN under FTZ.
inline constexpr float min_lms_response = 1e-20f;

}


}

// Colorspace operations

double
srgb_to_linear (uint8_t channel_8bit)
{
    return srgb().linear_d[channel_8bit];
}

// Björn Ottosson's OkLab forward transform — L channel only, since
// that's all percentile_lightness needs.
double
oklab_lightness (double r_linear, double g_linear, double b_linear)
{
    using namespace oklab;
    const auto &M1 = lms_from_linear_srgb<double>;

    const double l_ = std::cbrt(dot3(M1[0], r_linear, g_linear, b_linear));
    const double m_ = std::cbrt(dot3(M1[1], r_linear, g_linear, b_linear));
    const double s_ = std::cbrt(dot3(M1[2], r_linear, g_linear, b_linear));

    return dot3(lightness_from_lms_cbrt<double>, l_, m_, s_);
}

namespace
{

// x > 0. FreeBSD cbrtf bit-trick seed (~3% error) + one Halley step.
[[nodiscard]] inline float
cbrt_halley (float x) noexcept
{
    const float y  = std::bit_cast<float>(std::bit_cast<uint32_t>(x) / 3u + 709958130u);
    const float y3 = y * y * y;
    return y * (y3 + 2.0f * x) / (2.0f * y3 + x);
}

// Branch-free OkLab L for n contiguous Format_RGB32 pixels.
void
lightness_run (const QRgb *__restrict px, float *__restrict out, int n) noexcept
{
    using namespace oklab;
    const float *lut = srgb().linear_f.data();
    const auto &M1 = lms_from_linear_srgb<float>;

    for (int i = 0; i < n; ++i) {
        const float r = lut[qRed(px[i])];
        const float g = lut[qGreen(px[i])];
        const float b = lut[qBlue(px[i])];

        using namespace oklab;

        // Same constants as oklab_lightness(); that one stays as the reference.z
        const float l = std::max(dot3(M1[0], r, g, b), min_lms_response);
        const float m = std::max(dot3(M1[1], r, g, b), min_lms_response);
        const float s = std::max(dot3(M1[2], r, g, b), min_lms_response);

        out[i] = dot3(lightness_from_lms_cbrt<float>,
                      cbrt_halley(l), cbrt_halley(m), cbrt_halley(s));
    }
}

// Value at the given percentile of [first, first + count). Requires count > 0.
// Reorders the range.
template <typename T>
[[nodiscard]] T
select_percentile (T *first, size_t count, int percentile) noexcept
{
    const int p = std::clamp(percentile, 0, 100);
    const auto rank = static_cast<std::ptrdiff_t>((count - 1) * static_cast<size_t>(p) / 100);

    std::nth_element(first, first + rank, first + count);
    return first[rank];
}

// reach of DualKawaseBlur { passes, offset } as a fraction of the width of the texture it starts from
// 1080 is a good default just to smooth down a wall of noise.
constexpr double blur_sigma_fraction = std::numbers::sqrt2 * 1.5 * 16 / 1080;

struct run     { int x_begin = 0, x_end = 0; };
struct run_set { std::array<run, 2> r{}; int n = 0; };

}

ring_lightness
percentile_lightness (const QImage &chunk, int percentile,
                      double ring_crop_begin, double ring_crop_end)
{
    ring_lightness result_ring { .value = default_lightness_value,
                            .ring_crop_begin = ring_crop_begin,
                            .ring_crop_end = ring_crop_end };

    if (chunk.isNull() || chunk.width() <= 0 || chunk.height() <= 0) {
        return result_ring;
    }

    const int p = std::clamp(percentile, 0, 100);

    const double begin = std::clamp(ring_crop_begin, 0.0, 1.0);
    const double end   = std::clamp(ring_crop_end,   0.0, 1.0);
    if (begin >= end) {
        return result_ring; // empty ring
    }

    // cover thumbnails are treated as opaque; Format_RGB32 gives a known,
    // tightly-packed 0xffRRGGBB layout we can walk with a raw pointer.
    const QImage rgb = chunk.format() == QImage::Format_RGB32
        ? chunk
        : chunk.convertToFormat(QImage::Format_RGB32);

    const int full_w = rgb.width();
    const int full_h = rgb.height();

    // Bounding box of the ring's *outer* edge only — nothing past `end`
    // could ever pass the per-pixel test below, so there's no reason to
    // even visit it. Same formula the old center_inset crop used.
    const double outer_inset = (1.0 - end) / 2.0;
    const int x0 = static_cast<int>(full_w * outer_inset);
    const int y0 = static_cast<int>(full_h * outer_inset);
    const int w  = std::max(1, full_w - 2 * x0);
    const int h  = std::max(1, full_h - 2 * y0);

    const double half_w = full_w / 2.0;
    const double half_h = full_h / 2.0;
    const double inv_half_w = half_w > 0.0 ? 1.0 / half_w : 0.0;
    const double inv_half_h = half_h > 0.0 ? 1.0 / half_h : 0.0;

    // Column runs passing the x-side of the ring test. lo is the lower bound
    // on nx: `begin` for rows that cross the centre hole (ny < begin), 0 for
    // rows already beyond it. Half-open [begin, end) semantics, with end == 1
    // keeping the literal edge pixels, are preserved exactly.
    auto column_runs = [&](double lo) {
        run_set set;
        int start = -1;
        for (int x = x0; x <= x0 + w; ++x) {
            bool in = false;
            if (x < x0 + w) {
                const double nx = std::abs(x - half_w) * inv_half_w;
                in = nx >= lo && (end >= 1.0 || nx < end);
            }
            if (in && start < 0) {
                start = x;
            } else if (!in && start >= 0) {
                assert(set.n < 2); // monotone in |x - half_w| => at most two runs
                set.r[set.n++] = { start, x };
                start = -1;
            }
        }
        return set;
    };

    const run_set runs_hole = column_runs(begin); // rows with ny <  begin
    const run_set runs_full = column_runs(0.0);   // rows with ny >= begin

    // Upper bound, left uninitialised: lightness_run writes before nth_element reads.
    const auto buffer = std::make_unique_for_overwrite<float[]>(
        static_cast<size_t>(w) * static_cast<size_t>(h));
    size_t count = 0;

    for (int y = y0; y < y0 + h; ++y) {
        const double ny = std::abs(y - half_h) * inv_half_h;
        if (end < 1.0 && ny >= end) {
            continue; // t >= ny >= end: the whole row is outside the ring
        }

        const run_set &runs = ny < begin ? runs_hole : runs_full;
        const auto *row = reinterpret_cast<const QRgb *>(rgb.constScanLine(y));

        for (int i = 0; i < runs.n; ++i) {
            const int len = runs.r[i].x_end - runs.r[i].x_begin;
            lightness_run(row + runs.r[i].x_begin, buffer.get() + count, len);
            count += static_cast<size_t>(len);
        }
    }

    if (count == 0) {
        return result_ring;
    }

    const float result = select_percentile(buffer.get(), count, p);

    result_ring.value = std::clamp(static_cast<double>(result), 0.0, 1.0);

    return result_ring;
}

double
pondered_lightness (const cover_rings &rings)
{
    double weighted_sum = 0.0;
    double total_weight = 0.0;

    for (const ring_lightness &ring : rings) {
        const double begin = std::clamp(ring.ring_crop_begin, 0.0, 1.0);
        const double end   = std::clamp(ring.ring_crop_end,   0.0, 1.0);

        /*  Area fraction of a Chebyshev ring is exactly end^2 - begin^2. max(0, ...)
            so a malformed ring (begin >= end) contributes zero weight instead of
            a negative one. */
        const double weight = std::max(0.0, end * end - begin * begin);

        weighted_sum += ring.value * weight;
        total_weight += weight;
    }

    if (total_weight <= 0.0) {
        return default_lightness_value; // every ring contributed zero area -- neutral over undefined
    }

    return std::clamp(weighted_sum / total_weight, 0.0, 1.0);
}

namespace
{

/*  Pixel boundary of the index-th of `cells` equal divisions of `extent`.
    edge(0) == 0 and edge(cells) == extent, so [edge(i), edge(i+1)) tile
    [0, extent) exactly; neighbouring cells differ by at most one pixel.
    */
[[nodiscard]] constexpr int
cell_edge (size_t index, size_t cells, size_t extent) noexcept
{
    return static_cast<int>(static_cast<uint64_t>(index) * extent / cells);
}

/*  Half-open range of whole cells touched by [begin, end), given in cell
    units. Rounds outward so every touched cell counts. The tolerance keeps an
    interval that is cell-aligned up to floating-point noise from dragging in
    a neighbouring cell. The result is never empty and stays inside [0, cells].
    */
[[nodiscard]] std::pair<size_t, size_t>
touched_cells (double begin, double end, size_t cells) noexcept
{
    constexpr double tolerance = 1e-9;
    const auto last = static_cast<double>(cells);

    const auto first = static_cast<size_t>(
        std::clamp(std::floor(begin + tolerance), 0.0, last - 1.0));
    const auto past = static_cast<size_t>(
        std::clamp(std::ceil(end - tolerance), static_cast<double>(first + 1), last));

    return { first, past };
}

/*  How much of the cover's width and height a surface of this size shows when
    the cover fills it, centered and cropped to the surface's aspect ratio. One
    of the two is exactly 1.0. Empty if either aspect ratio is degenerate.
    */
[[nodiscard]] QSizeF
visible_cover_fraction (transform::ratio cover_aspect, QSize surface) noexcept
{
    const auto [aspect_w, aspect_h] = transform::find_aspect_ratio(
        static_cast<size_t>(surface.width()),
        static_cast<size_t>(surface.height()));

    const QSizeF cover_area(static_cast<double>(cover_aspect.first),
                            static_cast<double>(cover_aspect.second));

    const QSizeF fit = transform::largest_aspect_size(cover_area, aspect_w, aspect_h);
    if (fit.isEmpty()) {
        return {};
    }

    return { fit.width() / cover_area.width(), fit.height() / cover_area.height() };
}

// Separable Gaussian, clamp-to-edge.
template <size_t N>
void
blur_in_place (std::array<std::array<double, N>, N> &grid, double sigma)
{
    const int radius = std::max(1, static_cast<int>(std::ceil(3.0 * sigma)));
    std::vector<double> kernel(static_cast<size_t>(2 * radius + 1));
    double norm = 0.0;
    for (int i = -radius; i <= radius; ++i)
        norm += kernel[static_cast<size_t>(i + radius)] = std::exp(-0.5 * i * i / (sigma * sigma));
    for (double &k : kernel) k /= norm;

    const int n = static_cast<int>(N);

    // `at(line, i)` addresses element i of a line; called once per axis
    auto pass = [&](auto at) {
        std::array<double, N> out;
        for (int line = 0; line < n; ++line) {
            for (int i = 0; i < n; ++i) {
                double acc = 0.0;
                for (int k = -radius; k <= radius; ++k)
                    acc += kernel[static_cast<size_t>(k + radius)] * at(line, std::clamp(i + k, 0, n - 1));
                out[static_cast<size_t>(i)] = acc;
            }
            for (int i = 0; i < n; ++i) at(line, i) = out[static_cast<size_t>(i)];
        }
    };

    pass([&](int line, int i) -> double & { return grid[line][i]; });  // rows
    pass([&](int line, int i) -> double & { return grid[i][line]; });  // columns
}

}

lightness_table
lightness_table_of_image (const QImage &image)
{
    // Zeroed: the same "no data" value percentile_lightness gives an empty chunk.
    lightness_table table{};

    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        return table;
    }

    table.m_aspect_ratio = transform::find_aspect_ratio(
        static_cast<size_t>(image.width()),
        static_cast<size_t>(image.height()));

    // Convert once, so every task can walk the pixels as a raw 0xffRRGGBB buffer.
    const QImage rgb = image.format() == QImage::Format_RGB32
        ? image
        : image.convertToFormat(QImage::Format_RGB32);

    const size_t     width  = static_cast<size_t>(rgb.width());
    const size_t     height = static_cast<size_t>(rgb.height());
    const uchar     *base   = rgb.constBits();
    const qsizetype  stride = rgb.bytesPerLine();

    std::array<std::array<double, lightness_table::s_columns>, lightness_table::s_rows> cells;

    auto evaluate_row = [&](int row_index) {
        const auto row = static_cast<size_t>(row_index);

        const int y0 = cell_edge(row, lightness_table::s_rows, height);
        const int y1 = std::max(cell_edge(row + 1, lightness_table::s_rows, height), y0 + 1);
        const size_t band_h = static_cast<size_t>(y1 - y0);

        // 1. Lightness of the whole pixel band of this table row, computed once.
        //    The cells never overlap, so no pixel is converted twice.
        const auto band = std::make_unique_for_overwrite<float[]>(band_h * width);

        for (size_t y = 0; y < band_h; ++y) {
            const auto *src = reinterpret_cast<const QRgb *>(
                base + (static_cast<qsizetype>(y0) + static_cast<qsizetype>(y)) * stride);

            lightness_run(src, band.get() + y * width, static_cast<int>(width));
        }

        for (size_t column = 0; column < lightness_table::s_columns; ++column) {
            const int x0 = cell_edge(column, lightness_table::s_columns, width);
            const int x1 = std::max(cell_edge(column + 1, lightness_table::s_columns, width), x0 + 1);
            const auto cell_w = static_cast<size_t>(x1 - x0);

            double sum = 0.0;
            for (size_t y = 0; y < band_h; ++y) {
                const float *line = band.get() + y * width + static_cast<size_t>(x0);
                sum += std::accumulate(line, line + cell_w, 0.0);
            }

            cells[row][column] = sum / static_cast<double>(band_h * cell_w);
        }
    };

    std::array<int, lightness_table::s_rows> rows;
    std::iota(rows.begin(), rows.end(), 0);

    // Blocks until every row is done, so `rgb` outlives all the raw pointers.
    QtConcurrent::blockingMap(rows.begin(), rows.end(), evaluate_row);

    blur_in_place(cells, blur_sigma_fraction * lightness_table::s_columns);

    for (size_t r = 0; r < lightness_table::s_rows; ++r) {
        double running = 0.0;
        for (size_t c = 0; c < lightness_table::s_columns; ++c) {
            running += cells[r][c];
            table.m_sat[r + 1][c + 1] = table.m_sat[r][c + 1] + running;
        }
    }

    return table;
}

double
lightness_table::backdrop_lightness_for_rect (QRect rect, QSize reference_surface) const
{
    // Same "no data" value percentile_lightness gives an empty chunk.
    constexpr double no_backing = 0.0;

    if (reference_surface.isEmpty()) {
        return no_backing;
    }

    // NOTE: rect.x() and rect.y() are relative to the top-left corner of
    // reference_surface, NOT of the table. (0, 0) is the surface's corner.
    // The surface only covers part of the table (see below), so mixing up
    // the two origins is what shifts every lookup.
    //
    // Only the part of rect inside the surface has a visible backdrop.
    const QRect visible = rect.intersected(QRect(QPoint(0, 0), reference_surface));
    if (visible.isEmpty()) {
        return no_backing;
    }

    // 1. Logically fit the surface inside the table's area (the image's
    //    aspect ratio). Nothing is cropped or resized: this only computes
    //    what fraction of the table the surface shows per axis.
    const QSizeF shown = visible_cover_fraction(m_aspect_ratio, reference_surface);
    if (shown.isEmpty()) {
        return no_backing; // default-constructed table (ratio 0:0)
    }

    // 2. Express the fitted surface in cell units. The grid spans the whole
    //    table, centered like in the drawing.
    const double cells_w = shown.width()  * s_columns;
    const double cells_h = shown.height() * s_rows;

    const double origin_x = (static_cast<double>(s_columns) - cells_w) / 2.0;
    const double origin_y = (static_cast<double>(s_rows)    - cells_h) / 2.0;

    // surface pixels -> cells
    const double cells_per_px_x = cells_w / reference_surface.width();
    const double cells_per_px_y = cells_h / reference_surface.height();

    // 3. Locate rect from the surface's corner, then shift by the surface's
    //    own offset inside the table.
    const double left   = visible.x();
    const double top    = visible.y();
    const double right  = left + visible.width();   // exclusive edge
    const double bottom = top  + visible.height();

    const auto [col_begin, col_end] = touched_cells(origin_x + left  * cells_per_px_x,
                                                    origin_x + right * cells_per_px_x,
                                                    s_columns);
    const auto [row_begin, row_end] = touched_cells(origin_y + top    * cells_per_px_y,
                                                    origin_y + bottom * cells_per_px_y,
                                                    s_rows);

    // mean of the spanned cells, O(1)
    const double sum = m_sat[row_end][col_end]   - m_sat[row_begin][col_end]
                     - m_sat[row_end][col_begin] + m_sat[row_begin][col_begin];

    return sum / static_cast<double>((row_end - row_begin) * (col_end - col_begin));
}

}
