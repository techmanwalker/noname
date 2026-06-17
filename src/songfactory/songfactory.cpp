#include "songfactory.hpp"

#include <QFutureWatcher>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QtConcurrent/QtConcurrent>
#include <qmediaplayer.h>

// clean constructor for both qfuture and callback variants
song_factory::song_factory(const QUrl &source, std::shared_ptr<cover_provider> provider)
    : m_source(source),
      m_cover_provider(provider)
{}

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
    return QtConcurrent::run([source, provider](QPromise<Types::Song> &promise) {
        song_factory worker(source, provider);
        Types::Song result = worker.execute_extraction(promise);

        if (!promise.isCanceled()) {
            promise.addResult(result);
        }
    });
}

Types::Song
song_factory::execute_extraction(QPromise<Types::Song> &promise)
{
    QMediaPlayer mediaPlayer;
    QEventLoop loop;
    Types::Song song;

    if (promise.isCanceled()) {
        return {};
    }

    // Stop if promise is canceled

    QFutureWatcher<Types::Song> watcher;
    watcher.setFuture(promise.future());

    QObject::connect(&watcher, &QFutureWatcher<Types::Song>::canceled, &loop, &QEventLoop::quit);

    // local event loop of this thread will stop when the status changes
    connect(&mediaPlayer, &QMediaPlayer::mediaStatusChanged, &loop, [&loop, &mediaPlayer, &song, this](QMediaPlayer::MediaStatus status) {

        if (status == QMediaPlayer::LoadedMedia) {

            // support audio and video, and reject everything that has no decodable audio
            if (!mediaPlayer.hasAudio()) {
                song = Types::Song(); // empty struct
                loop.quit();
                return;
            }

            song.source = m_source;

            const QMediaMetaData meta = mediaPlayer.metaData();
            
            song.duration = static_cast<quint64>(mediaPlayer.duration());
            song.title = meta.value(QMediaMetaData::Title).toString();
            song.artist = meta.value(QMediaMetaData::ContributingArtist).toString();
            song.album = meta.value(QMediaMetaData::AlbumTitle).toString();

            if (song.title.isEmpty()) {
                song.title = m_source.fileName();
            }

            // use m_cover_provider to store the covers and generate uris
            QString cover_uid = "";
            
            if (m_cover_provider != nullptr) {
                cover_uid = m_cover_provider->store(meta.value(QMediaMetaData::ThumbnailImage));
            }

            if (!cover_uid.isEmpty()) {
                song.cover = cover_provider::schema + cover_uid;
            } else {
                song.cover = QUrl(QString(m_cover_provider->default_cover_uri));
            }
            
            loop.quit();
        } else if (status == QMediaPlayer::InvalidMedia) {
            loop.quit();
        }
    });

    mediaPlayer.setSource(m_source);
    
    // if metadata is not immediately ready, process the events of this worker thread
    if (mediaPlayer.mediaStatus() != QMediaPlayer::LoadedMedia && 
        mediaPlayer.mediaStatus() != QMediaPlayer::InvalidMedia) {
        if (promise.isCanceled()) {
            return {};
        }

        loop.exec();
    }

    if (promise.isCanceled()) {
        return {};
    }

    return song;
}

/// Async load metadata of songs
QFuture<QList<Types::Song>>
song_factory::batch_extract(const QList<QUrl> &sources, std::shared_ptr<cover_provider> provider)
{
    // Build a list of the metadata extraction requests for each source
    QList<QFuture<Types::Song>> requests;
    requests.reserve(sources.size());

    for (const QUrl &source : sources) {
        requests.append(
            song_factory::extract(source, provider)
        );
    }

    // Monitor the entire batch without blocking the main thread
    // Pass the iterators and explicit type to whenAll.
    // Only pass the lambda expression as single argument to .then().
    // The parameter 'extracted_songs_ready_to_append' will contain the identical
    // list we built as input, ready to get the results from.
    return QtConcurrent::run([requests = std::move(requests)]() mutable {
        QList<Types::Song> extracted_songs_ready_to_append;
        extracted_songs_ready_to_append.reserve(requests.size());

        // .waitForFinished() en cada futuro detiene el hilo del pool de forma segura 
        // hasta que el MediaStatusChanged y el bucle de eventos de CADA canción hayan muerto limpiamente.
        for (auto &future : requests) {
            future.waitForFinished(); 
            extracted_songs_ready_to_append.append(future.result());
        }

        return extracted_songs_ready_to_append;
    });
}
