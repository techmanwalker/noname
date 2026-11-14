#include "presentation/lightness.hpp"

#include <QImage>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

using namespace covers::lightness;
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
    return percentile_lightness(img, 50).value;
}

static QImage
filled (int w, int h, QRgb c) { QImage i(w, h, QImage::Format_RGB32); i.fill(c); return i; }

static bool
table_tests () {
    bool failed = false;

    //  A constant image gives that constant under any rect: catches SAT indexing and normalization.
    {
        const auto table = lightness_table_of_image(filled(300, 300, qRgb(128, 128, 128)));
        const double want = reference_L(128, 128, 128);
        for (QRect r : { QRect(0, 0, 1000, 1000), QRect(333, 71, 12, 640), QRect(900, 900, 100, 100) })
            failed |= std::abs(table.backdrop_lightness_for_rect(r, QSize(1000, 1000)) - want) > 1e-3;
    }

    //  Black | white split on a square cover. Far sides read ~0 and ~1;
    //    a rect centered on the edge reads 0.5 (blurred step, symmetric window).
    {
        QImage img = filled(512, 512, qRgb(0, 0, 0));
        for (int y = 0; y < 512; ++y)
            for (int x = 256; x < 512; ++x) img.setPixel(x, y, qRgb(255, 255, 255));

        const auto table = lightness_table_of_image(img);
        const QSize s(1000, 1000);
        failed |= table.backdrop_lightness_for_rect({  50, 400, 100, 100 }, s) > 1e-3;
        failed |= table.backdrop_lightness_for_rect({ 850, 400, 100, 100 }, s) < 1.0 - 1e-3;
        failed |= std::abs(table.backdrop_lightness_for_rect({ 450, 400, 100, 100 }, s) - 0.5) > 1e-3;
    }

    //  Aspect mapping: a 2:1 cover (black | white | black) seen through a square surface shows
    //    only its center half. The surface's left edge sits on the white/black boundary, so a rect
    //    just inside it must read bright. If the mapping ignored the aspect, it would read ~0.
    {
        QImage img = filled(200, 100, qRgb(0, 0, 0));
        for (int y = 0; y < 100; ++y)
            for (int x = 50; x < 150; ++x) img.setPixel(x, y, qRgb(255, 255, 255));

        const auto table = lightness_table_of_image(img);
        failed |= table.backdrop_lightness_for_rect({ 100, 400, 100, 200 }, QSize(1000, 1000)) < 0.9;
    }

    return failed;
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

    return (worst < 1e-4 ? 0 : 1) || table_tests();
}
