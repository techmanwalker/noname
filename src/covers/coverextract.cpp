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

double
percentile_luminance (const QImage &cover, int percentile, double center_inset)
{
    if (cover.isNull() || cover.width() <= 0 || cover.height() <= 0) {
        return 0.0;
    }

    const int p = std::clamp(percentile, 0, 100);
    // clamped below 0.5 so there's always at least a sliver of image left
    const double inset = std::clamp(center_inset, 0.0, 0.49);

    // Cover thumbnails are treated as opaque; Format_RGB32 gives a known,
    // tightly-packed 0xffRRGGBB layout we can walk with a raw pointer.
    const QImage rgb = cover.format() == QImage::Format_RGB32
        ? cover
        : cover.convertToFormat(QImage::Format_RGB32);

    const int full_w = rgb.width();
    const int full_h = rgb.height();

    const int x0 = static_cast<int>(full_w * inset);
    const int y0 = static_cast<int>(full_h * inset);
    const int w  = std::max(1, full_w - 2 * x0);
    const int h  = std::max(1, full_h - 2 * y0);

    std::vector<double> lightness;
    lightness.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

    for (int y = y0; y < y0 + h; ++y) {
        const auto *row = reinterpret_cast<const QRgb *>(rgb.constScanLine(y));
        for (int x = x0; x < x0 + w; ++x) {
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

}
}