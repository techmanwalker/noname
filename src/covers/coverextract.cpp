#include "coverextract.hpp"

#include <QMutex>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

namespace {

/*  QImage::fromData() triggers Qt's own image-format plugin loading
    (QFactoryLoader) the first several times a given picture format is
    decoded — a process-wide, lazily-populated registry that isn't safe to
    race from multiple threads simultaneously. Every worker thread in this
    pool calls this for cover art, so the decode step gets serialized —
    same fix as before, just living here again now that Qt is doing the
    decoding instead of FFmpeg/swscale. */
QMutex image_decode_mutex;

/* `square` must already be width == height. Returns a null QImage on
   swscale context failure (e.g. unsupported CPU path — practically never
   happens, but don't crash on it). */
QImage
lanczos_resize_square(const QImage &square, int target_size)
{
    const QImage src = square.convertToFormat(QImage::Format_RGBA8888);

    // qCDebug (song_factory::l_songfactory()) << "Attempting to rescale a cover. Source size: " << src.width() << "x" << src.height();

    SwsContext *sws = sws_getContext(
        src.width(), src.height(), AV_PIX_FMT_RGBA,
        target_size, target_size, AV_PIX_FMT_RGBA,
        SWS_LANCZOS, nullptr, nullptr, nullptr);
    if (!sws) {
        return QImage();
    }

    QImage dst(target_size, target_size, QImage::Format_RGBA8888);

    const uint8_t *src_slices[1] = { src.constBits() };
    int src_strides[1] = { static_cast<int>(src.bytesPerLine()) };
    uint8_t *dst_slices[1] = { dst.bits() };
    int dst_strides[1] = { static_cast<int>(dst.bytesPerLine()) };

    sws_scale(sws, src_slices, src_strides, 0, src.height(), dst_slices, dst_strides);
    sws_freeContext(sws);

    /* qCDebug (song_factory::l_songfactory()) << "Rescaling completed. Source size: " << src.width() << "x" << src.height()
        << "; destination size: " << dst.width() << "x" << dst.height();*/

    return dst;
}

} // namespace





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
    QImage cover;

    {
        QMutexLocker locker(&image_decode_mutex);
        cover = QImage::fromData(reinterpret_cast<const uchar *>(data.data()), static_cast<int>(data.size()));
    }

    if (cover.isNull()) {
        return QImage();
    }

    const int side = std::min(cover.width(), cover.height());
    cover = cover.copy((cover.width() - side) / 2, (cover.height() - side) / 2, side, side);

    if (crop_and_resize != 0 && static_cast<size_t>(side) != crop_and_resize) {
        cover = lanczos_resize_square(cover, static_cast<int>(crop_and_resize));
    }

    return cover;
}

}
}