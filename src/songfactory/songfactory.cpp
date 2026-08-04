#include "songfactory.hpp"
#include "coverprovider.hpp"
#include "thumbnails.hpp"

#include <QCryptographicHash>
#include <QLoggingCategory>

#include <QtConcurrent/QtConcurrent>

#include <cstddef>
#include <qloggingcategory.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}


Q_LOGGING_CATEGORY(song_factory::l_songfactory, "noname.songfactory");

namespace {

/*  QImage::fromData() triggers Qt's own image-format plugin loading
    (QFactoryLoader) the first several times a given picture format is
    decoded — a process-wide, lazily-populated registry that isn't safe to
    race from multiple threads simultaneously. Every worker thread in this
    pool calls this for cover art, so the decode step gets serialized —
    same fix as before, just living here again now that Qt is doing the
    decoding instead of FFmpeg/swscale. */
QMutex image_decode_mutex;

QString
to_qstring (const TagLib::String &s)
{
    return QString::fromStdString(s.to8Bit(true)); // true = UTF-8
}

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

QImage
extract_cover (TagLib::File *file, song_factory::attributes a)
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

    if (a.crop_and_resize != 0 && static_cast<size_t>(side) != a.crop_and_resize) {
        cover = lanczos_resize_square(cover, static_cast<int>(a.crop_and_resize));
    }

    return cover;
}

} // namespace

// No need for std::optional at the moment. A Song without source is already invalid.

// clean constructor for both qfuture and callback variants
song_factory::song_factory(const QUrl &source, std::shared_ptr<cover_provider> provider)
    : m_source(source),
      m_cover_provider(provider)
{}

QThreadPool*
song_factory::extraction_pool()
{
    // C++11/20 guarantees thread-safe initialization of static local variables.
    static QThreadPool pool;
    
    /*  Optional: YThrottle the maximum thread count here to prevent 
        extreme disk I/O thrashing on mechanical drives. 
        If left untouched, it defaults to QThread::idealThreadCount().
        pool.setMaxThreadCount(4);  */
    
    return &pool;
}

/**
    @brief Extract song metadata asynchronously and expose the
    result wrapped in a QFuture.

    @note Supports .result() without blocking player events, as the extraction 
          and QMediaPlayer lifecycle run entirely on a dedicated worker thread.
*/
QFuture<Types::Song>
song_factory::extract(const QUrl &source, std::shared_ptr<cover_provider> provider, attributes a)
{
    // delegate instantiation and execution to Qt thread pool
    return QtConcurrent::run(extraction_pool(), [source, provider, a](QPromise<Types::Song> &promise) {
        song_factory worker(source, provider);
        Types::Song result = worker.execute_extraction(promise, a);

        if (!promise.isCanceled()) {
            if (!result.is_valid()) {
                qCDebug(l_songfactory) << "Extraction promise for source \"" << source << "\" returned an invalid song.";
            }
            promise.addResult(result);
        } else {
            qCDebug(l_songfactory) << "Extraction promise was canceled for source \"" << source << "\".";
        }
    });
}

Types::Song
song_factory::execute_extraction(QPromise<Types::Song> &promise, attributes a)
{
    Types::Song song;

    if (promise.isCanceled()) {
        return {};
    }

    const QByteArray local_path = m_source.toLocalFile().toUtf8();

    TagLib::FileRef file(local_path.constData());

    if (file.isNull() || !file.tag()) {
        qCWarning(l_songfactory) << "Could not open" << m_source << "for metadata extraction.";
        return {};
    }

    TagLib::AudioProperties *props = file.audioProperties();

    // TagLib has no explicit "hasAudio" flag — this is the closest real signal
    // it gives you: channels()/sampleRate() describe whatever audio track it
    // found. A video file with no audio at all should leave both at zero;
    // treat that the same way the old mediaPlayer.hasAudio() /
    // av_find_best_stream(AVMEDIA_TYPE_AUDIO, ...) checks did.
    if (!props || props->channels() <= 0 || props->sampleRate() <= 0) {
        qCWarning(l_songfactory) << "File \"" << m_source << "\" does not seem to feature a valid audio track.";

        return {};
    }

    TagLib::Tag *tag = file.tag();

    song.source = m_source;
    song.title  = to_qstring(tag->title());
    song.artist = to_qstring(tag->artist());
    song.album  = to_qstring(tag->album());

    if (song.title.isEmpty()) {
        song.title = m_source.fileName();
    }

    song.duration = static_cast<quint64>(props->lengthInMilliseconds());

    if (promise.isCanceled()) {
        return {};
    }

    if (m_cover_provider == nullptr) {
        qCWarning(l_songfactory) << "Cover provider was not set. No covers will be appended.";

        if (promise.isCanceled()) {
            return {};
        }

        return song;
    }

    const QString thumb_hash = thumbnail_hash_for(m_source, a.crop_and_resize);

    // if the thumbnail is cached, cover_provider will handle it, but if not, manual store is triggered
    if (!localdata::has_thumbnail(thumb_hash)) {
        if (!m_cover_provider->store(thumb_hash, extract_cover(file.file(), a))) {
            qCWarning (l_songfactory) << "Cover was retrieved from audio file, but was unable to store in memory cache.";
        };
    }

    // the cover provider will handle the rest
    song.cover = cover_provider::schema + thumb_hash;

    if (promise.isCanceled()) {
        return {};
    }

    return song;
}

/*  Deterministic, session-independent cache key for a song's thumbnail.
    Hashing the absolute path (not file contents) keeps this cheap — an MD5
    over a path-length string is microseconds, dwarfed by the decode/encode
    it lets us skip. crop_and_resize is folded in too, since the cache
    stores the already-resized image: without it, two call sites requesting
    different sizes for the same song would collide on one cache entry and
    silently serve the wrong resolution to whichever asked second. */
QString
song_factory::thumbnail_hash_for (const QUrl &source, size_t crop_and_resize)
{
    const QByteArray key = source.toLocalFile().toUtf8()
                          + ':' + QByteArray::number(qulonglong(crop_and_resize));

    return QString::fromLatin1(
        QCryptographicHash::hash(key, QCryptographicHash::Md5).toHex());
}

void
song_factory::shutdown()
{
    QThreadPool *pool = extraction_pool();
    pool->clear();          // drop anything queued but not yet started
    pool->waitForDone();    // let whatever's already running finish naturally
}