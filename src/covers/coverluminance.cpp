#include "coverluminance.hpp"

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

namespace covers::luminance
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

[[nodiscard]] const srgb_tables &
srgb ()
{
    // safe to call from other static initializers and from any worker thread
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

double
oklab_lightness (double r_linear, double g_linear, double b_linear)
{
    // Björn Ottosson's OkLab forward transform — L channel only, since
    // that's all percentile_luminance needs.
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
// Reorders the range. Same rank formula as before, now in one place.
template <typename T>
[[nodiscard]] T
select_percentile (T *first, size_t count, int percentile) noexcept
{
    const int p = std::clamp(percentile, 0, 100);
    const auto rank = static_cast<std::ptrdiff_t>((count - 1) * static_cast<size_t>(p) / 100);

    std::nth_element(first, first + rank, first + count);
    return first[rank];
}

struct run     { int x_begin = 0, x_end = 0; };
struct run_set { std::array<run, 2> r{}; int n = 0; };

}

ring_luma
percentile_luminance (const QImage &chunk, int percentile,
                      double ring_crop_begin, double ring_crop_end)
{
    if (chunk.isNull() || chunk.width() <= 0 || chunk.height() <= 0) {
        return {};
    }

    const int p = std::clamp(percentile, 0, 100);

    const double begin = std::clamp(ring_crop_begin, 0.0, 1.0);
    const double end   = std::clamp(ring_crop_end,   0.0, 1.0);
    if (begin >= end) {
        return {}; // empty ring
    }

    // Cover thumbnails are treated as opaque; Format_RGB32 gives a known,
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
        return {};
    }

    const float result = select_percentile(buffer.get(), count, p);

    return {
        .value=std::clamp(static_cast<double>(result), 0.0, 1.0),
        .ring_crop_begin=ring_crop_begin,
        .ring_crop_end=ring_crop_end
    };
}

double
pondered_luma (const cover_rings &lumas)
{
    double weighted_sum = 0.0;
    double total_weight = 0.0;

    for (const ring_luma &ring : lumas) {
        const double begin = std::clamp(ring.ring_crop_begin, 0.0, 1.0);
        const double end   = std::clamp(ring.ring_crop_end,   0.0, 1.0);

        // See the header comment: area fraction of a Chebyshev ring is
        // exactly end^2 - begin^2. max(0, ...) so a malformed ring
        // (begin >= end) contributes zero weight instead of a negative one.
        const double weight = std::max(0.0, end * end - begin * begin);

        weighted_sum += ring.value * weight;
        total_weight += weight;
    }

    if (total_weight <= 0.0) {
        return 0.5; // every ring contributed zero area -- neutral over undefined
    }

    return std::clamp(weighted_sum / total_weight, 0.0, 1.0);
}

namespace
{

// Pixel boundary of the index-th of `cells` equal divisions of `extent`.
// edge(0) == 0 and edge(cells) == extent, so [edge(i), edge(i+1)) tile
// [0, extent) exactly; neighbouring cells differ by at most one pixel.
[[nodiscard]] constexpr int
cell_edge (size_t index, size_t cells, size_t extent) noexcept
{
    return static_cast<int>(static_cast<uint64_t>(index) * extent / cells);
}

// Half-open range of whole cells touched by [begin, end), given in cell
// units. Rounds outward so every touched cell counts. The tolerance keeps an
// interval that is cell-aligned up to floating-point noise from dragging in
// a neighbouring cell. The result is never empty and stays inside [0, cells].
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

}

table_luma
luma_table_of_image (const QImage &image, int percentile)
{
    // Zeroed: the same "no data" value percentile_luminance gives an empty chunk.
    table_luma table{};

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

    // Widest cell of any table row: ceil(width / s_columns) <= width / s_columns + 1.
    const size_t max_cell_w = width / table_luma::s_columns + 1;

    // One task = one table row. Each task writes only its own row of
    // m_table and owns its buffers, so there is nothing to lock.
    auto evaluate_row = [&](int row_index) {
        const auto row = static_cast<size_t>(row_index);

        const int y0 = cell_edge(row, table_luma::s_rows, height);
        const int y1 = std::max(cell_edge(row + 1, table_luma::s_rows, height), y0 + 1);
        const size_t band_h = static_cast<size_t>(y1 - y0);

        // 1. Lightness of the whole pixel band of this table row, computed once.
        //    Full-width scanlines are long contiguous runs, which is what the
        //    vectorized kernel needs. The cells never overlap, so no pixel is
        //    converted twice.
        const auto band = std::make_unique_for_overwrite<float[]>(band_h * width);

        for (size_t y = 0; y < band_h; ++y) {
            const auto *src = reinterpret_cast<const QRgb *>(
                base + (static_cast<qsizetype>(y0) + static_cast<qsizetype>(y)) * stride);

            lightness_run(src, band.get() + y * width, static_cast<int>(width));
        }

        // 2. One percentile per cell, gathered from the band. nth_element
        //    reorders its input, so each cell is copied into a scratch buffer
        //    that is allocated once per task and reused for all of its cells.
        const auto scratch = std::make_unique_for_overwrite<float[]>(band_h * max_cell_w);

        for (size_t column = 0; column < table_luma::s_columns; ++column) {
            const int x0 = cell_edge(column, table_luma::s_columns, width);
            const int x1 = std::max(cell_edge(column + 1, table_luma::s_columns, width), x0 + 1);
            const auto cell_w = static_cast<size_t>(x1 - x0);

            float *out = scratch.get();
            for (size_t y = 0; y < band_h; ++y) {
                const float *line = band.get() + y * width + static_cast<size_t>(x0);
                out = std::copy_n(line, cell_w, out);
            }

            const size_t count = band_h * cell_w; // >= 1: both extents are forced to >= 1 pixel

            // Same [0, 1] clamp percentile_luminance applies to its result.
            table.m_table[row][column] = std::clamp(
                static_cast<double>(select_percentile(scratch.get(), count, percentile)),
                0.0, 1.0);
        }
    };

    std::array<int, table_luma::s_rows> rows;
    std::iota(rows.begin(), rows.end(), 0);

    // Blocks until every row is done, so `rgb` outlives all the raw pointers.
    QtConcurrent::blockingMap(rows.begin(), rows.end(), evaluate_row);

    return table;
}

double
table_luma::backing_luma_for_rect (QRect rect, QSize reference_surface, int percentile) const
{
    // Same "no data" value percentile_luminance gives an empty chunk.
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

    // 1. Logically fit the surface inside the table's area. The table
    //    covers the whole image, so its area is the image's aspect ratio.
    //    Nothing is cropped or resized: this only computes a size.
    const auto [aspect_w, aspect_h] = transform::find_aspect_ratio(
        static_cast<size_t>(reference_surface.width()),
        static_cast<size_t>(reference_surface.height()));

    const QSizeF table_area(static_cast<double>(m_aspect_ratio.first),
                            static_cast<double>(m_aspect_ratio.second));

    const QSizeF fit = transform::largest_aspect_size(table_area, aspect_w, aspect_h);
    if (fit.isEmpty()) {
        return no_backing; // default-constructed table (ratio 0:0)
    }

    // 2. Express the fitted surface in cell units. The grid spans the whole
    //    table, so the fitted surface is a fraction of it per axis (one of
    //    the two is exactly 1.0), centered like in the drawing.
    const double cells_w = fit.width()  / table_area.width()  * s_columns;
    const double cells_h = fit.height() / table_area.height() * s_rows;

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

    // 4. Percentile of the spanned cells (not the mean: 1 1 1 1 0 0 0 must
    //    not average into a gray that matches neither side).
    std::vector<double> values;
    values.reserve((row_end - row_begin) * (col_end - col_begin));

    for (size_t r = row_begin; r < row_end; ++r) {
        for (size_t c = col_begin; c < col_end; ++c) {
            values.push_back(m_table[r][c]);
        }
    }

    // Same rank formula as percentile_luminance. `values` is never empty.
    const int p = std::clamp(percentile, 0, 100);
    const auto rank = static_cast<std::ptrdiff_t>(
        (values.size() - 1) * static_cast<size_t>(p) / 100);

    std::nth_element(values.begin(), values.begin() + rank, values.end());

    return values[static_cast<size_t>(rank)];
}

}