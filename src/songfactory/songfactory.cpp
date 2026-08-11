#include "songfactory.hpp"
#include "coverextract.hpp"
#include "coverprovider.hpp"
#include "thumbnails.hpp"

#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QUuid>

#include <QtConcurrent/QtConcurrent>

#include <taglib/fileref.h>
#include <taglib/tag.h>


namespace {

QString
to_qstring (const TagLib::String &s)
{
    return QString::fromStdString(s.to8Bit(true)); // true = UTF-8
}

}


Q_LOGGING_CATEGORY(song_factory::l_songfactory, "noname.songfactory");

// No need for std::optional at the moment. A Song without source is already invalid.

// clean constructor for both qfuture and callback variants
song_factory::song_factory(const QUrl &source, std::shared_ptr<covers::live::cover_provider> provider)
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
song_factory::extract(const QUrl &source, std::shared_ptr<covers::live::cover_provider> provider, attributes a)
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

    using namespace covers::live;

    if (a.use_thumbnail_cache) {
        const QString thumb_hash = covers::disk::thumbnail_hash_for(m_source, a.crop_and_resize);

        /*  No decode here anymore — just tell cover_provider where to find
            the cover if/when it's actually requested (e.g. the song scrolls
            into view). Cheap regardless of whether a disk thumbnail already
            exists; keeps the registry correct if the disk cache was ever
            cleared externally. */
        m_cover_provider->register_source(thumb_hash, m_source, a.crop_and_resize);

        // the cover provider will handle the rest with the path hash
        song.cover = cover_provider::schema + thumb_hash;
    } else {

        // generate a unique uuid to not ever touch a real thumbnail

        QString cover_uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);

        qCDebug (l_songfactory) << "no cache thumbnail uuid: " << cover_uuid;

        bool success = m_cover_provider->store(
            cover_uuid, 
            covers::live::extract_cover(file.file(), a.crop_and_resize), // always extract from audio file
            false // do not save to disk cache, must be fetched by in memory cache
        );

        if (!success) {
            qCWarning (l_songfactory) << "Cover was retrieved from audio file, but was unable to store in memory cache.";
        }

        // the cover provider never really cared about the hash if it is from memory
        song.cover = cover_provider::schema + cover_uuid;

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