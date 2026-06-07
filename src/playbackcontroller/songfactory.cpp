#include "playbackcontroller.hpp"
#include "songfactory.hpp"

void 
song_factory::extract(const QUrl &source, then callback)
 {
    // To store the cover in the cover_provider cache
    auto shared_provider = playback_controller::instance().m_cover_provider;

    // Will be automatically destroyed when this task ends
    auto *extractor = new song_factory(source, callback, shared_provider);
    extractor->start();
}

song_factory::song_factory(
    const QUrl &source, 
    then callback, 
    std::shared_ptr<cover_provider> provider)

    : m_source(source),
      m_callback(callback),
      m_cover_provider(provider)
{
    // We DON'T connect a QAudioOutput to m_media_player.
    // Since it has no audio output, it won't play sound nor affect the global audio system
    connect(&m_media_player, &QMediaPlayer::mediaStatusChanged, this, &song_factory::handle_media_status_changed);
}

void
song_factory::start()
{
    m_media_player.setSource(m_source);
}

void
song_factory::handle_media_status_changed(QMediaPlayer::MediaStatus status) 
{
    if (status == QMediaPlayer::LoadedMedia) {
        const QMediaMetaData meta = m_media_player.metaData();
        Types::Song song;
        song.source = m_source;
        song.duration = static_cast<quint64>(m_media_player.duration()); // QMediaDecoder provides the length in ms
        song.title = meta.value(QMediaMetaData::Title).toString();
        song.artist = meta.value(QMediaMetaData::ContributingArtist).toString();
        song.album = meta.value(QMediaMetaData::AlbumTitle).toString();

        if (song.title.isEmpty()) {
            song.title = m_source.fileName();
        }

        // Use m_cover_provider for cover loading.
        QString cover_uid = m_cover_provider->store(meta.value(QMediaMetaData::ThumbnailImage));
        if (!cover_uid.isEmpty()) {
            song.cover = cover_provider::schema + cover_uid;
        } else {
            // If it came empty or was not a valid image, use the default
            song.cover = QUrl(QString(m_cover_provider->default_cover_uri));
        }
        
        m_callback(song);
        deleteLater(); // safe auto teardown on the event loop
    }
}

void
song_factory::handle_error()
{
    // return a basic Song struct with the filename in case of failure
    Types::Song fallback_song;
    fallback_song.source = m_source;
    fallback_song.title = m_source.fileName();
    
    m_callback(fallback_song);
    deleteLater();
}