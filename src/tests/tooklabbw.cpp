// tooklchbw <input img> <output img>
//
// Renders an image as the OkLab lightness of every pixel: decoded through the
// covers ffmpeg tooling, written as a 16 bpc grayscale-in-RGB PNG.

#include "decodeedit/decode.hpp"
#include "decodeedit/pixelformats.hpp"
#include "presentation/lightness.hpp"

#include <CLI/CLI.hpp>

#include <QFile>
#include <QImage>
#include <QRgba64>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

namespace {

// Inverse of the OkLab L channel for an achromatic color (a = b = 0):
// L = cbrt(Y), so Y = L^3, then the sRGB transfer curve. Feeding the result
// back to oklab_lightness() returns L, and a gray input maps to itself.
[[nodiscard]] quint16
gray16_from_oklab_lightness (double lightness)
{
    const double l = std::clamp(lightness, 0.0, 1.0);
    const double linear = l * l * l;
    const double encoded = linear <= 0.0031308
        ? linear * 12.92
        : 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;

    return static_cast<quint16>(std::lround(std::clamp(encoded, 0.0, 1.0) * 65535.0));
}

// `rgba` must be pixelformat_qimage (8 bpc R,G,B,A). Alpha is ignored, same as
// the cover lightness pipeline, which treats covers as opaque.
[[nodiscard]] QImage
oklab_lightness_image (const QImage &rgba)
{
    if (rgba.isNull() || rgba.format() != pixelformat_qimage)
        return {};

    QImage out(rgba.size(), QImage::Format_RGBX64);
    if (out.isNull())
        return {};

    const uchar *src_base = rgba.constBits();
    const qsizetype src_stride = rgba.bytesPerLine();
    uchar *dst_base = out.bits();
    const qsizetype dst_stride = out.bytesPerLine();
    const int width = rgba.width();

    std::vector<int> rows(static_cast<size_t>(rgba.height()));
    std::iota(rows.begin(), rows.end(), 0);

    // Rows are independent and each task writes only its own row.
    QtConcurrent::blockingMap(rows.begin(), rows.end(), [&](int y) {
        const uchar *src = src_base + y * src_stride;
        auto *dst = reinterpret_cast<QRgba64 *>(dst_base + y * dst_stride);

        for (int x = 0; x < width; ++x, src += 4) {
            using namespace covers::lightness;

            const double l = oklab_lightness(srgb_to_linear(src[0]),
                                             srgb_to_linear(src[1]),
                                             srgb_to_linear(src[2]));
            const quint16 v = gray16_from_oklab_lightness(l);

            dst[x] = QRgba64::fromRgba64(v, v, v, 0xffff);
        }
    });

    return out;
}

} // namespace

int
main (int argc, char *argv[])
{
    CLI::App app{"Render an image as its OkLab lightness (16 bpc PNG)", "tooklchbw"};

    std::string input_path;
    std::string output_path;
    app.add_option("input", input_path, "Input image")
        ->required()->check(CLI::ExistingFile);
    app.add_option("output", output_path, "Output PNG path")
        ->required();

    CLI11_PARSE(app, argc, argv);

    QFile file(QFile::decodeName(input_path.c_str()));
    if (!file.open(QIODevice::ReadOnly)) {
        std::fprintf(stderr, "cannot open '%s'\n", input_path.c_str());
        return 1;
    }
    const QByteArray bytes = file.readAll();

    const QImage decoded = covers::decode::decode_cover_ffmpeg(
        reinterpret_cast<const uchar *>(bytes.constData()),
        static_cast<size_t>(bytes.size()),
        pixelformat_qimage);
    if (decoded.isNull()) {
        std::fprintf(stderr, "cannot decode '%s'\n", input_path.c_str());
        return 1;
    }

    const QImage result = oklab_lightness_image(decoded);
    if (result.isNull()) {
        std::fprintf(stderr, "lightness conversion failed\n");
        return 1;
    }

    if (!result.save(QFile::decodeName(output_path.c_str()), "PNG")) {
        std::fprintf(stderr, "cannot write '%s'\n", output_path.c_str());
        return 1;
    }

    std::printf("%dx%d -> %s\n", result.width(), result.height(), output_path.c_str());
    return 0;
}
