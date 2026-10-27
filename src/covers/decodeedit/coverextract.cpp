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

    // complexProperties("PICTURE") must be called on the File, not the Tag
    // for broader audio file format compatibility (the latter does not work for FLAC)
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
    //      This internally computes largest_aspect_size.
    const QImage cropped = transform::crop_largest_aspect(
        frame, requested_size.width(), requested_size.height());

    // 8-9. Single Lanczos pass on the cropped region.
    return transform::lanczos_resize(cropped,
                                     static_cast<size_t>(requested_size.width()),
                                     static_cast<size_t>(requested_size.height()));
}

}
}