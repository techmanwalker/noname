#include "playbackcontroller.hpp"
#include "songfactory.hpp"
#include <QMediaMetaData>
#include <QMediaPlayer>
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

    // raw high frequency signal, unused
    connect(&m_media_player, &QMediaPlayer::positionChanged,
            this, &playback_controller::position_changed);

    connect(&m_media_player, &QMediaPlayer::durationChanged,
            this, &playback_controller::handle_duration_changed);

    connect(&m_media_player, &QMediaPlayer::mediaStatusChanged,
            this, &playback_controller::handle_media_status_changed);

    connect(&m_media_player, &QMediaPlayer::playbackStateChanged,
            this, &playback_controller::playback_state_changed);

    // Assuming that PlaybackPresentation converted this class in the
    // signal sender, let's do an autoconnect
    connect(this, &playback_controller::r_duration_slider_pressed_changed,
            this, &playback_controller::handle_duration_slider_pressed_changed);
}

void
playback_controller::play()
{
    m_media_player.play();

    // Playback change signal is automatically sent by QMediaPlayer
    // and connected on the constructor
}

void
playback_controller::pause()
{
    m_media_player.pause();
}

void
playback_controller::stop()
{
    m_media_player.stop();
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
    stop();
    m_media_player.setSource(QUrl());
    
    // Reset atómico de la estructura local a valores por defecto (no nulos)
    // Atomic reset of the local struct to default values (not null)
    m_current_track = Types::Song{};
    m_status = metadata_load_status::idle;

    emit track_changed();
    emit metadata_status_changed();
    emit duration_changed();
}

void
playback_controller::set_position(const quint64 position_ms)
{
    m_media_player.setPosition(static_cast<qint64>(position_ms));
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
    song_factory::extract(m_media_player.source()).then([this](Types::Song loaded_track) {
        m_current_track = loaded_track;
        m_status = metadata_load_status::ready;
        emit track_changed();
        emit metadata_status_changed();
    });
}

void
playback_controller::handle_duration_slider_pressed_changed(bool pressed)
{
    // Yet another reactive binding

    // if a drag started
    if (pressed) {
        playback_state_when_last_slider_drag_started = playback_state();
        pause(); // you don't want it to sound while it is being dragged
    } else {
        // restore the playback to where it was
        switch (playback_state_when_last_slider_drag_started) {
            case QMediaPlayer::PlayingState:
                play(); // if it was playing, start playing again
                break;
            case QMediaPlayer::PausedState:
                pause();
                break;
            case QMediaPlayer::StoppedState:
                stop();
                break;

            // no default this time
        }
    }
}