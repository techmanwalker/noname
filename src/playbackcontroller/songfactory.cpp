#include "playbackcontroller.hpp"
#include "songfactory.hpp"

#include <QFutureWatcher>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QtConcurrent/QtConcurrent>

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
song_factory::extract(const QUrl &source)
{
    auto shared_provider = playback_controller::instance().m_cover_provider;

    // delegate instantiation and execution to Qt thread pool
    return QtConcurrent::run([source, shared_provider]() {
        song_factory worker(source, shared_provider);
        return worker.execute_extraction();
    });
}

Types::Song
song_factory::execute_extraction()
{
    QMediaPlayer mediaPlayer;
    QEventLoop loop;
    Types::Song song;
    
    // safe initial fallback
    song.source = m_source;
    song.title = m_source.fileName();

    // local event loop of this thread will stop when the status changes
    connect(&mediaPlayer, &QMediaPlayer::mediaStatusChanged, &loop, [&loop, &mediaPlayer, &song, this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            const QMediaMetaData meta = mediaPlayer.metaData();
            
            song.duration = static_cast<quint64>(mediaPlayer.duration());
            song.title = meta.value(QMediaMetaData::Title).toString();
            song.artist = meta.value(QMediaMetaData::ContributingArtist).toString();
            song.album = meta.value(QMediaMetaData::AlbumTitle).toString();

            if (song.title.isEmpty()) {
                song.title = m_source.fileName();
            }

            // use m_cover_provider to store the covers and generate uris
            QString cover_uid = m_cover_provider->store(meta.value(QMediaMetaData::ThumbnailImage));
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
        loop.exec();
    }

    return song;
}