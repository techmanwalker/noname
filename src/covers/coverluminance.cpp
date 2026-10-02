#include "coverluminance.hpp"

#include <array>
#include <cmath>

#include <QImage>

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

// after
double
percentile_luminance (const QImage &cover, int percentile,
                       double ring_crop_begin, double ring_crop_end)
{
    if (cover.isNull() || cover.width() <= 0 || cover.height() <= 0) {
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
    const QImage rgb = cover.format() == QImage::Format_RGB32
        ? cover
        : cover.convertToFormat(QImage::Format_RGB32);

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
pondered_luma (const cover_luma &lumas)
{
    // list all the ring lumas here since reflection still does not exist
    const luma *rings[] = { &lumas.nuclear_luma, &lumas.centric_luma,
                             &lumas.midring_luma, &lumas.borders_luma };

    double weighted_sum = 0.0;
    double total_weight = 0.0;

    for (const luma *ring : rings) {
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

}