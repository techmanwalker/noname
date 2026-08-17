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

}
}