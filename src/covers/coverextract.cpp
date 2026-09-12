#include "coverdecode.hpp"
#include "coverextract.hpp"
#include "pixelformats.hpp"

namespace covers {
namespace live {

QImage
extract_cover (TagLib::File *file, size_t crop_and_resize)
{
    if (!file) {
        return QImage();
    }

    // complexProperties("PICTURE") must be called on the File, not the Tag —
    // for most formats the base File implementation just forwards to the
    // Tag's own complexProperties(), but FLAC::File overrides this
    // specifically, because FLAC's cover art (METADATA_BLOCK_PICTURE) is a
    // top-level container block, not part of the tag itself. Calling this
    // on tag() instead skips that override and silently returns nothing
    // for every FLAC file, regardless of what's actually embedded.
    TagLib::List<TagLib::VariantMap> pictures = file->complexProperties("PICTURE");
    if (pictures.isEmpty()) {
        return QImage();
    }

    const TagLib::VariantMap &picture = pictures.front();

    auto it = picture.find("data");
    if (it == picture.end() || it->second.isEmpty()) {
        return QImage();
    }

    TagLib::ByteVector data = it->second.value<TagLib::ByteVector>();
    if (data.isEmpty()) {
        return QImage();
    }
    QImage cover = decode::decode_cover_ffmpeg(
        reinterpret_cast<const uchar*>(data.data()), 
        data.size(), 
        pixelformat_qimage //[cite: 16]
    );

    if (cover.isNull()) {
        return QImage();
    }

    if (crop_and_resize != 0) {
        cover = decode::lanczos_resize_square(cover, static_cast<int>(crop_and_resize));
    }

    return cover;
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
            const double r = decode::srgb_to_linear(static_cast<uint8_t>(qRed(px)));
            const double g = decode::srgb_to_linear(static_cast<uint8_t>(qGreen(px)));
            const double b = decode::srgb_to_linear(static_cast<uint8_t>(qBlue(px)));
            lightness.push_back(decode::oklab_lightness(r, g, b));
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

namespace {
// Fit offline (scikit-learn LogisticRegression) against the initial
// hand-labeled ring-luma survey, not hand-tuned like the older curve stops.
// Refit and replace all four wholesale if the survey grows; don't
// hand-edit any one of them individually, they only mean anything together.
constexpr double kWeightCentric = 1.1631627190035592;
constexpr double kWeightBorders = 2.0557630549973074;
constexpr double kWeightMidring = 2.8449702280884748;
constexpr double kBias          = -4.75136611;
} // anonymous

double
probability_light (const cover_luma &lumas)
{
    const double z = kWeightCentric * lumas.centric_luma
                    + kWeightBorders * lumas.borders_luma
                    + kWeightMidring * lumas.midring_luma
                    + kBias;
    return 1.0 / (1.0 + std::exp(-z));
}

}
}