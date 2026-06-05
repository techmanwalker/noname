#include "playbackcontroller.hpp"
#include <QMediaMetaData>
#include <qmediaplayer.h>

// Meyers singleton implementation
playback_controller &
playback_controller::instance()
{
    static playback_controller s_instance;
    return s_instance;
}

// Private constructor
playback_controller::playback_controller(QObject *parent)
    : QObject(parent)
{
    m_media_player.setAudioOutput(&m_audio_output);

    polling_position_timer.setInterval(polling_position_timer_interval);

    connect(&m_media_player, &QMediaPlayer::positionChanged,
            this, &playback_controller::position_changed);

    // For safe position tracking from the GUI, poll with a timer
    connect(&polling_position_timer, &QTimer::timeout,
            this, &playback_controller::position_poll_requested);

    connect(&m_media_player, &QMediaPlayer::durationChanged,
            this, &playback_controller::handle_duration_changed);

    connect(&m_media_player, &QMediaPlayer::mediaStatusChanged,
            this, &playback_controller::handle_media_status_changed);

    connect(&m_media_player, &QMediaPlayer::playbackStateChanged,
            this, &playback_controller::handle_playback_state_changed);
}

void
playback_controller::play()
{
    m_media_player.play();

    // Playback change signal is automatically sent by QMediaPlayer
    // and connected on the constructor

    // keep synced
    // position_changed() is automagically reemitted
    emit position_poll_requested();
}

void
playback_controller::pause()
{
    m_media_player.pause();

    emit position_poll_requested();
}

void
playback_controller::load(const QUrl &source)
{
    if (m_current_track.source == source) {
        return;
    }

    // 1. Increase the transaction ID. Any previous operation becomes obsolete.
    m_current_transaction_id++;
    
    // 2. Immediate status transition: system knows that the current data is not valid anymore.
    m_status = metadata_load_status::loading_metadata;
    emit metadata_status_changed();

    // 3. Make the audio backend load the track.
    m_media_player.setSource(source);
}

void
playback_controller::unload()
{
    m_current_transaction_id++; // invalidate pending loads
    m_media_player.stop();
    m_media_player.setSource(QUrl());
    
    // Reset atómico de la estructura local a valores por defecto (no nulos)
    // Atomic reset of the local struct to default values (not null)
    m_current_track = Types::Song{};
    m_status = metadata_load_status::idle;

    emit track_changed();
    emit metadata_status_changed();
    emit position_poll_requested();
    emit duration_changed();
}

void
playback_controller::set_position(const quint64 position_ms)
{
    m_media_player.setPosition(static_cast<qint64>(position_ms));

    emit position_poll_requested();
}

void
playback_controller::set_volume(quint8 volume_percent)
{
    // Force upper limit
    if (volume_percent > 100) {
        volume_percent = 100;
    }

    // Convert the 0..100 scale to 0.0f..1.0f for QAudioOutput
    float volume_float = static_cast<float>(volume_percent) / 100.0f;
    m_audio_output.setVolume(volume_float);

    // Emit signal
    emit volume_changed();
}

Types::Song
playback_controller::current_track() const
{
    return m_current_track;
}

quint8
playback_controller::current_volume() const
{
    // Fetch 0-1 volume value
    float volume_float = m_audio_output.volume();

    // Scale up to 0-100
    return static_cast<quint8>(qRound(volume_float * 100.0f));
}

quint64
playback_controller::current_position_ms() const
{
    qint64 position = m_media_player.position();
    
    // If Qt reports an invalid or negative transition state, return 0 safely
    if (position < 0) {
        return 0;
    }
    
    return static_cast<quint64>(position);
}

QMediaPlayer::PlaybackState
playback_controller::playback_state() const
{
    return m_media_player.playbackState();
}


QMediaPlayer::MediaStatus
playback_controller::media_status() const
{
    return m_media_player.mediaStatus();
}

metadata_load_status
playback_controller::metadata_status() const
{
    return m_status;
}

bool
playback_controller::is_loading() const
{
    return m_status == metadata_load_status::loading_metadata;
}

void
playback_controller::handle_duration_changed()
{
    emit duration_changed();
}

void
playback_controller::handle_media_status_changed()
{
    if (media_status() == QMediaPlayer::LoadedMedia) {
        // Locally save the ID that triggered this slot in the execution stack
        const uint64_t transaction_at_callback = m_current_transaction_id;

        const QMediaMetaData meta_data = m_media_player.metaData();
        
        // Build the temporary isolated container on the stack
        Types::Song loaded_track;
        loaded_track.source = m_media_player.source();
        loaded_track.duration = static_cast<quint64>(m_media_player.duration());
        loaded_track.title = meta_data.value(QMediaMetaData::Title).toString();
        loaded_track.artist = meta_data.value(QMediaMetaData::Author).toString();
        loaded_track.album = meta_data.value(QMediaMetaData::AlbumTitle).toString();
        loaded_track.cover = meta_data.value(QMediaMetaData::ThumbnailImage).toUrl();

        if (loaded_track.title.isEmpty()) {
            loaded_track.title = loaded_track.source.fileName();
        }

        // RACE CONTROL:
        // If current transaction_id changed while Qt processed the track,
        // this means that the user called load() for another song. Discard.
        if (transaction_at_callback != m_current_transaction_id) {
            return; 
        }

        // 100% atomic replace, synced to the main thread
        m_current_track = loaded_track; 
        m_status = metadata_load_status::ready;

        // Notify the ViewModel the whole block of metadata at once
        emit track_changed();
        emit metadata_status_changed();
    }
    else if (media_status() == QMediaPlayer::InvalidMedia) {
        m_status = metadata_load_status::idle;
        emit metadata_status_changed();
    }
}

void
playback_controller::handle_playback_state_changed()
{
    emit playback_state_changed();
    
    // Reactive bind to the throttled position changed QTimer
    // to avoid repeating start() and stop() a bazillion times
    switch (playback_state()) {
        case QMediaPlayer::PlayingState:
            emit position_poll_requested();

            if (!polling_position_timer.isActive()) {
                polling_position_timer.start();
            }
            break;
        default:
            emit position_poll_requested();
            
            if (polling_position_timer.isActive()) {
                polling_position_timer.stop();
            }
            break;
    }
}