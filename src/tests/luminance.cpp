#include "coverluminance.hpp"

#include <QImage>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

using namespace covers::luminance;
using namespace covers::transform;

static double
reference_L (int r, int g, int b)
{
    const double l = oklab_lightness(srgb_to_linear(r), srgb_to_linear(g), srgb_to_linear(b));
    return std::clamp(l, 0.0, 1.0);
}

static double
kernel_L (int r, int g, int b)
{
    QImage img(1, 1, QImage::Format_RGB32);
    img.setPixel(0, 0, qRgb(r, g, b));
    return percentile_luminance(img, 50).value;
}

int
main ()
{
    double worst = 0.0;
    int wr = 0, wg = 0, wb = 0;

    auto check = [&](int r, int g, int b) {
        const double err = std::abs(kernel_L(r, g, b) - reference_L(r, g, b));
        if (err > worst) { worst = err; wr = r; wg = g; wb = b; }
    };

    for (int v = 0; v < 256; ++v) check(v, v, v);               // gray ramp

    for (int r = 0; r < 16; ++r)                                 // dark cube: where the 1e-20f floor matters
        for (int g = 0; g < 16; ++g)
            for (int b = 0; b < 16; ++b)
                check(r, g, b);

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> byte(0, 255);
    for (int i = 0; i < 200000; ++i) check(byte(rng), byte(rng), byte(rng));

    std::printf("max abs error %.3e at (%d, %d, %d)\n", worst, wr, wg, wb);

    bool failed_ring_pick_test = false;

    {
        using enum luma_types;

        cover_rings rings;
        ring_of(rings, nuclear) = { 0.1, 0.00, 0.15 };
        ring_of(rings, centric) = { 0.2, 0.15, 0.60 };
        ring_of(rings, midring) = { 0.3, 0.60, 0.80 };
        ring_of(rings, borders) = { 0.4, 0.80, 1.00 };

        const QSize surface(1000, 1000);
        const ratio square{ 1, 1 };

        auto expect = [&](QRect r, std::optional<luma_types> want) {
            if (dominant_ring_for_rect(rings, square, r, surface) != want) {
                std::printf("FAIL rect (%d,%d %dx%d)\n", r.x(), r.y(), r.width(), r.height());
                failed_ring_pick_test = true;
            }
        };

        expect({ 480, 480,  40,  40 }, nuclear);
        expect({ 300, 300, 100, 100 }, centric);
        expect({ 120, 400,  60, 100 }, midring);
        expect({  10,  10,  60,  60 }, borders);
        expect({  50, 400, 100, 100 }, midring);       // exact 50/50 tie: inner ring wins
        expect({ 950, 400, 100, 100 }, borders);       // clipped to the surface
        expect({ 1100, 400,  50,  50 }, std::nullopt); // entirely outside
    }

    return (worst < 1e-4 ? 0 : 1) || failed_ring_pick_test;
}