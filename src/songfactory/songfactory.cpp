#include "songfactory.hpp"

#include <QtConcurrent/QtConcurrent>

#include <qloggingcategory.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

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

QImage
extract_cover (TagLib::File *file)
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

    QMutexLocker locker(&image_decode_mutex);
    return QImage::fromData(reinterpret_cast<const uchar *>(data.data()), static_cast<int>(data.size()));
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
song_factory::extract(const QUrl &source, std::shared_ptr<cover_provider> provider)
{
    // delegate instantiation and execution to Qt thread pool
    return QtConcurrent::run(extraction_pool(), [source, provider](QPromise<Types::Song> &promise) {
        song_factory worker(source, provider);
        Types::Song result = worker.execute_extraction(promise);

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
song_factory::execute_extraction(QPromise<Types::Song> &promise)
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

    QImage cover_image = extract_cover(file.file());

    QString cover_uid = "";

    if (m_cover_provider == nullptr) {
        qCWarning(l_songfactory) << "Cover provider was not set. No covers will be appended.";
    } else {
        cover_uid = m_cover_provider->store(cover_image);

        if (cover_uid.isEmpty()) {
            qCDebug (l_songfactory) << "No cover uuid returned. Falling back to default...";
            song.cover = QUrl(QString(m_cover_provider->default_cover_uri));
        } else {
            song.cover = cover_provider::schema + cover_uid;
        }
    }

    if (promise.isCanceled()) {
        return {};
    }

    return song;
}

void
song_factory::shutdown()
{
    QThreadPool *pool = extraction_pool();
    pool->clear();          // drop anything queued but not yet started
    pool->waitForDone();    // let whatever's already running finish naturally
}