#include "audioengine.hpp"

#include <QFuture>
#include <QMediaMetaData>
#include <QMediaPlayer>

// Meyers singleton implementation
audio_engine &
audio_engine::instance()
{
    static audio_engine s_instance;
    return s_instance;
}

// Private constructor
audio_engine::audio_engine(QObject *parent)
    : QObject(parent)
{
    m_media_player.setAudioOutput(&m_audio_output);

    // raw high frequency signal, unused
    connect(&m_media_player, &QMediaPlayer::positionChanged,
            this, &audio_engine::position_changed);

    connect(&m_media_player, &QMediaPlayer::durationChanged,
            this, &audio_engine::handle_duration_changed);

    connect(&m_media_player, &QMediaPlayer::playbackStateChanged,
            this, &audio_engine::playback_state_changed);

    connect(&m_media_player, &QMediaPlayer::mediaStatusChanged,
            this, &audio_engine::handle_media_status_changed);

    // Assuming that PlayerPresenter converted this class in the
    // signal sender, let's do an autoconnect
    connect(this, &audio_engine::r_duration_slider_pressed_changed,
            this, &audio_engine::handle_duration_slider_pressed_changed);

    connect(&queue, &PlayQueue::playheadChanged,
            this, &audio_engine::handle_playhead_changed);
}

void
audio_engine::play()
{
    m_media_player.play();

    // Playback change signal is automatically sent by QMediaPlayer
    // and connected on the constructor
}

void
audio_engine::pause()
{
    m_media_player.pause();
}

void
audio_engine::stop()
{
    m_media_player.stop();
}

QFuture<void>
audio_engine::load (const Types::Song &song) // song IS the metadata, no need to async wait
{
    if (song.source.isEmpty()) return QtFuture::makeReadyVoidFuture();

    // allow starting over without reloading the song
    if (m_media_player.source() == song.source) {
        m_media_player.setPosition(0);
        
        return QtFuture::makeReadyVoidFuture();
    }

    auto solve_when_loaded = std::make_shared<QPromise<void>>();
    auto future = solve_when_loaded->future();

    m_media_player.setSource(song.source);

    auto do_after_loading = [this, song, solve_when_loaded](const QMediaPlayer::MediaStatus &status) {
        switch (status) {
            // track changed
            case QMediaPlayer::LoadedMedia:
                m_current_track = song;
                queue.switch_to(song);
                emit track_changed();
                solve_when_loaded->finish();
                break;
            case QMediaPlayer::InvalidMedia:
                solve_when_loaded->finish();
                break;
            default:
                
                break; // todo: yet to be defined
        }

        solve_when_loaded->finish();
    };

    connect(
        &m_media_player, 
        &QMediaPlayer::mediaStatusChanged, 
        this, 
        do_after_loading, 
        Qt::SingleShotConnection
    );

    return future;
}

void
audio_engine::unload()
{
    m_current_transaction_id++; // invalidate pending loads
    stop();
    m_media_player.setSource(QUrl());
    
    m_current_track = Types::Song{};

    emit track_changed();
    emit duration_changed();
}

void
audio_engine::set_position(const quint64 position_ms)
{
    m_media_player.setPosition(static_cast<qint64>(position_ms));
}

void
audio_engine::set_volume(quint8 volume_percent)
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
audio_engine::current_track() const
{
    return m_current_track;
}

quint8
audio_engine::current_volume() const
{
    // Fetch 0-1 volume value
    float volume_float = m_audio_output.volume();

    // Scale up to 0-100
    return static_cast<quint8>(qRound(volume_float * 100.0f));
}

quint64
audio_engine::current_position_ms() const
{
    qint64 position = m_media_player.position();
    
    // If Qt reports an invalid or negative transition state, return 0 safely
    if (position < 0) {
        return 0;
    }
    
    return static_cast<quint64>(position);
}

QMediaPlayer::PlaybackState
audio_engine::playback_state() const
{
    return m_media_player.playbackState();
}


QMediaPlayer::MediaStatus
audio_engine::media_status() const
{
    return m_media_player.mediaStatus();
}

void
audio_engine::handle_duration_changed()
{
    emit duration_changed();
}

void
audio_engine::handle_duration_slider_pressed_changed(bool pressed)
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

void
audio_engine::handle_media_status_changed()
{
    if (m_media_player.mediaStatus() == QMediaPlayer::EndOfMedia) {
        queue.next();
    }
}

void
audio_engine::handle_playhead_changed(bool play_afterwards)
{
    QModelIndex current_index = queue.playhead();

    if (!current_index.isValid()) return;

    // get the Types::Any
    const Types::Any &media_item = queue.itemAt(current_index.row());

    // narrow down to Types::Song
    if (std::holds_alternative<Types::Song>(media_item)) {
        const Types::Song &song = std::get<Types::Song>(media_item);
        load(song);
        if (play_afterwards) play();
    }
}