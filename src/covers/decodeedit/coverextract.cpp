#include "coverdecode.hpp"
#include "coverextract.hpp"
#include "pixelformats.hpp"

namespace covers {
namespace live {

QImage
extract_cover (TagLib::File *file)
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
        pixelformat_qimage
    );

    if (cover.isNull()) {
        return QImage();
    }

    return cover;
}

namespace {
// Fraction of the duration where the thumbnail frame is taken.
constexpr double thumbnail_position = 0.15;
}

QImage
make_thumbnail (const QUrl &file)
{
    // requested_size.{w|h} == 0 means "return at full size"
    if (!file.isLocalFile()) {
        return {};
    }

    return decode::decode_video_frame_at_fraction(
        file.toLocalFile(), thumbnail_position, pixelformat_qimage);
}

}
}