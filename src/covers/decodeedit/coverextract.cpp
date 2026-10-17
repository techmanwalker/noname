#include "coverdecode.hpp"
#include "covertransform.hpp"
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
        cover = transform::lanczos_resize_square(cover, static_cast<int>(crop_and_resize));
    }

    return cover;
}

namespace {
// Fraction of the duration where the thumbnail frame is taken.
constexpr double thumbnail_position = 0.15;
}

QImage
make_thumbnail (const QUrl &file, const QSize &requested_size)
{
    // requested_size.{w|h} == 0 means "return at full size"
    if (!file.isLocalFile()) {
        return {};
    }

    const QImage frame = decode::decode_video_frame_at_fraction(
        file.toLocalFile(), thumbnail_position, pixelformat_qimage);

    if (frame.isNull()) {
        return {};
    }

    if (requested_size.isEmpty()) return frame;

    // 6-7. Center-crop to the requested aspect ratio, still at native resolution.
    //      Passing w:h straight through is enough: largest_aspect_size compares
    //      by cross-multiplication, so there's no need to reduce it with
    //      find_aspect_ratio first.
    const QImage cropped = transform::crop_largest_aspect(
        frame, requested_size.width(), requested_size.height());

    // 8-9. Single Lanczos pass on the cropped region.
    return transform::lanczos_resize(cropped,
                                     static_cast<size_t>(requested_size.width()),
                                     static_cast<size_t>(requested_size.height()));
}

}
}