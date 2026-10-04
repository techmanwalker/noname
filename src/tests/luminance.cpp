#include "coverluminance.hpp"

#include <QImage>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

using namespace covers::luminance;

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
    return worst < 1e-4 ? 0 : 1;
}