#include "coverluminance.hpp"

#include <array>
#include <cmath>

#include <QImage>
#include <QtConcurrent/QtConcurrent>

namespace covers::luminance
{

// Colorspace operations

double
srgb_to_linear (uint8_t channel_8bit)
{
    static const std::array<double, 256> lut = [] {
        std::array<double, 256> table{};
        for (int i = 0; i < 256; ++i) {
            const double c = i / 255.0;
            table[i] = c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
        }
        return table;
    }();

    return lut[channel_8bit];
}

double
oklab_lightness (double r_linear, double g_linear, double b_linear)
{
    // Björn Ottosson's OkLab forward transform — L channel only, since
    // that's all percentile_luminance needs.
    const double l = 0.4122214708 * r_linear + 0.5363325363 * g_linear + 0.0514459929 * b_linear;
    const double m = 0.2119034982 * r_linear + 0.6806995451 * g_linear + 0.1073969566 * b_linear;
    const double s = 0.0883024619 * r_linear + 0.2817188376 * g_linear + 0.6299787005 * b_linear;

    const double l_ = std::cbrt(l);
    const double m_ = std::cbrt(m);
    const double s_ = std::cbrt(s);

    return 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_;
}

double
percentile_luminance (const QImage &chunk, int percentile,
                       double ring_crop_begin, double ring_crop_end)
{
    if (chunk.isNull() || chunk.width() <= 0 || chunk.height() <= 0) {
        return 0.0;
    }

    const int p = std::clamp(percentile, 0, 100);

    const double begin = std::clamp(ring_crop_begin, 0.0, 1.0);
    const double end   = std::clamp(ring_crop_end,   0.0, 1.0);
    if (begin >= end) {
        return 0.0; // empty ring
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

    std::vector<double> lightness;
    lightness.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

    for (int y = y0; y < y0 + h; ++y) {
        const auto *row = reinterpret_cast<const QRgb *>(rgb.constScanLine(y));
        const double ny = std::abs(y - half_h) * inv_half_h; // hoisted out of the x loop
        for (int x = x0; x < x0 + w; ++x) {
            const double nx = std::abs(x - half_w) * inv_half_w;
            const double t  = std::max(nx, ny);

            // half-open [begin, end) — except end == 1 must still keep the
            // literal outer-edge pixels, or a "borders" ring would exclude
            // its own border.
            if (t < begin || (end < 1.0 && t >= end)) {
                continue;
            }

            const QRgb px = row[x];
            const double r = srgb_to_linear(static_cast<uint8_t>(qRed(px)));
            const double g = srgb_to_linear(static_cast<uint8_t>(qGreen(px)));
            const double b = srgb_to_linear(static_cast<uint8_t>(qBlue(px)));
            lightness.push_back(oklab_lightness(r, g, b));
        }
    }

    if (lightness.empty()) {
        return 0.0;
    }

    // nth_element is O(n) average — reading one rank doesn't justify an
    // O(n log n) full sort.
    const auto rank = static_cast<std::vector<double>::difference_type>(
        (lightness.size() - 1) * static_cast<size_t>(p) / 100);
    std::nth_element(lightness.begin(), lightness.begin() + rank, lightness.end());

    return std::clamp(lightness[static_cast<size_t>(rank)], 0.0, 1.0);
}

double
pondered_luma (const cover_rings &lumas)
{
    // list all the ring lumas here since reflection still does not exist
    const ring_luma *rings[] = { &lumas.nuclear_luma, &lumas.centric_luma,
                             &lumas.midring_luma, &lumas.borders_luma };

    double weighted_sum = 0.0;
    double total_weight = 0.0;

    for (const ring_luma *ring : rings) {
        const double begin = std::clamp(ring->ring_crop_begin, 0.0, 1.0);
        const double end   = std::clamp(ring->ring_crop_end,   0.0, 1.0);
        // See the header comment: area fraction of a Chebyshev ring is
        // exactly end^2 - begin^2. max(0, ...) so a malformed ring
        // (begin >= end) contributes zero weight instead of a negative one.
        const double weight = std::max(0.0, end * end - begin * begin);

        weighted_sum += ring->value * weight;
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

    // Convert once. Every cell view below is already Format_RGB32, so
    // percentile_luminance skips its own per-call conversion.
    const QImage rgb = image.format() == QImage::Format_RGB32
        ? image
        : image.convertToFormat(QImage::Format_RGB32);

    const size_t     width  = static_cast<size_t>(rgb.width());
    const size_t     height = static_cast<size_t>(rgb.height());
    const uchar     *base   = rgb.constBits();
    const qsizetype  stride = rgb.bytesPerLine();

    // One task = one table row. Each task writes only its own row of
    // m_table, so there is nothing to lock.
    auto evaluate_row = [&](int row_index) {
        const auto row = static_cast<size_t>(row_index);

        const int y0 = cell_edge(row, table_luma::s_rows, height);
        const int y1 = std::max(cell_edge(row + 1, table_luma::s_rows, height), y0 + 1);

        for (size_t column = 0; column < table_luma::s_columns; ++column) {
            const int x0 = cell_edge(column, table_luma::s_columns, width);
            const int x1 = std::max(cell_edge(column + 1, table_luma::s_columns, width), x0 + 1);

            // Zero-copy, read-only view into `rgb`; no pixels are duplicated.
            const uchar *origin = base + y0 * stride
                                       + x0 * static_cast<qsizetype>(sizeof(QRgb));

            const QImage cell(origin, x1 - x0, y1 - y0, stride, QImage::Format_RGB32);

            table.m_table[row][column] = percentile_luminance(cell, percentile);
        }
    };

    std::array<int, table_luma::s_rows> rows;
    std::iota(rows.begin(), rows.end(), 0);

    // Blocks until every row is done, so `rgb` outlives all the views.
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