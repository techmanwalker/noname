#include "songfactoryimpl.hpp"
#include "mediatypes.hpp"

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


Q_LOGGING_CATEGORY(song_factory_impl::l_songfactory, "noname.songfactory");

// No need for std::optional at the moment. A Song without source is already invalid.

// clean constructor for both qfuture and callback variants
song_factory_impl::song_factory_impl(const QUrl &source)
    : m_source(source)
{}

QThreadPool*
song_factory_impl::extraction_pool()
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
song_factory_impl::extract(const QUrl &source, attributes a)
{
    // delegate instantiation and execution to Qt thread pool
    return QtConcurrent::run(extraction_pool(), [source, a](QPromise<Types::Song> &promise) {
        song_factory_impl worker(source);
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

QList<QFuture<Types::Song>>
song_factory_impl::progressive_extract (const QList<QUrl> &sources, attributes a)
{
    // Read the metadata of the sources one by one in parallel, you can do something as soon as one extraction finishes
    QList<QFuture<Types::Song>> requests;
    requests.reserve(sources.size());

    for (const QUrl &source : sources) {
        requests.append(song_factory_impl::extract(source, a));
    }

    return requests;
}

QFuture<QList<Types::Song>>
song_factory_impl::batch_extract (const QList<QUrl> &sources, attributes a)
{
    auto requests = progressive_extract(sources, a);

    // Wait for all songs in the list to finish metadata extraction

    return QtFuture::whenAll(
        requests.begin(),
        requests.end()
    )
    .then([](const QList<QFuture<Types::Song>> &finished) {
        QList<Types::Song> extracted_songs_ready_to_append;

        extracted_songs_ready_to_append.reserve(finished.size());

        for (const auto &f : finished) {
            /*  Isolate one bad future's exception to just this iteration — otherwise a single
                failed extraction throws out of this loop and takes the whole batch's already-
                successful results down with it, since QFuture::result() rethrows whatever
                exception QtConcurrent::run captured on that future's worker thread. */
            if (f.isCanceled()) {
                continue;
            }

            // truly exceptional metadata extraction failures
            try {
                extracted_songs_ready_to_append.append(f.result());
            } catch (const std::exception &e) {
                qCWarning(l_songfactory) << "Skipping one song in batch: extraction failed with" << e.what();
            } catch (...) {
                qCWarning(l_songfactory) << "Skipping one song in batch: extraction failed with an unknown exception.";
            }
        }

        return extracted_songs_ready_to_append;
    });
}

Types::Song
song_factory_impl::execute_extraction(QPromise<Types::Song> &promise, attributes a)
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

    // create the cover reference
    song.cover = CoverRef (m_source, a.crop_and_resize);

    if (promise.isCanceled()) {
        return {};
    }

    return song;
}

void
song_factory_impl::teardown()
{
    QThreadPool *pool = extraction_pool();
    pool->clear();          // drop anything queued but not yet started
    pool->waitForDone();    // let whatever's already running finish naturally
}