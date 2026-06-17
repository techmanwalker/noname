#pragma once

#include "mediatypes.hpp"
#include "playqueue.hpp"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>

/**
    @brief Playback controller proxy for whatever audio framework hides behind the scenes.

    @note Implemented this way so if the audio backend would ever change, the internal API
    stays mostly intact.

*/
class audio_engine : public QObject
{
    Q_OBJECT
public:
    // Disable copy and reassignment to guarantee single instance
    audio_engine(const audio_engine&) = delete;
    audio_engine &operator=(const audio_engine&) = delete;

    // Meyers singleton instance for global access within C++
    static audio_engine &instance();

    void play();
    void pause();
    void stop();
    void unload();
    void set_position(const quint64 position_ms);
    void set_volume(quint8 volume_percent);

    // Load process is synchronous on its call, but asynchronous on its resolution
    QFuture<void> load(const Types::Song &song);

    // Safe getters for current data status
    Types::Song current_track()       const;
    quint64     current_position_ms() const;
    quint8      current_volume()      const; // volume from 0 to 100
    QMediaPlayer::PlaybackState playback_state() const;
    QMediaPlayer::MediaStatus media_status()     const;

signals:
    void position_changed();
    void duration_changed();
    void volume_changed();
    
    // Single atomic signal to send the whole block of data at once
    void track_changed();
    void playback_state_changed();

    // Reverse signal from QML down to the controller
    void r_duration_slider_pressed_changed(bool pressed);

private slots:
    void handle_duration_changed();
    void handle_duration_slider_pressed_changed(bool pressed);
    void handle_media_status_changed();
    void handle_playhead_changed(bool play_afterwards = false); // triggered by a switch_to or click in QML

private:
    // Private constructor for the singleton pattern
    explicit audio_engine(QObject *parent = nullptr);
    ~audio_engine() override = default;

    QMediaPlayer m_media_player;
    QAudioOutput m_audio_output;

    // Protected internal status
    Types::Song m_current_track;

    QMediaPlayer::PlaybackState playback_state_when_last_slider_drag_started;
    
    // The core of synchronization: control of load versions
    uint64_t m_current_transaction_id = 0;

    PlayQueue &queue = PlayQueue::instance();
};