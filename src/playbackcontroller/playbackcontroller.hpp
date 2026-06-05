#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QString>
#include <QMediaPlayer>
#include <QTimer>

#include "abstractmodel.hpp" // for Types:: namespace

enum class metadata_load_status {
    idle,
    loading_metadata,
    ready
};

/**
    @brief Playback controller proxy for whatever audio framework hides behind the scenes.

    @note Implemented this way so if the audio backend would ever change, the internal API
    stays mostly intact.

*/
class playback_controller : public QObject
{
    Q_OBJECT
public:
    // Disable copy and reassignment to guarantee single instance
    playback_controller(const playback_controller&) = delete;
    playback_controller &operator=(const playback_controller&) = delete;

    // Meyers singleton instance for global access within C++
    static playback_controller &instance();

    void play();
    void pause();
    void unload();
    void set_position(const quint64 position_ms);
    void set_volume(quint8 volume_percent);

    // Load process is synchronous on its call, but asynchronous on its resolution
    void load(const QUrl &source);

    // Safe getters for current data status
    Types::Song current_track()       const;
    quint64     current_position_ms() const;
    quint8      current_volume()      const; // volume from 0 to 100
    QMediaPlayer::PlaybackState playback_state() const;
    QMediaPlayer::MediaStatus media_status()     const;
    metadata_load_status metadata_status()       const;
    bool is_loading() const;

signals:
    void position_changed();
    void position_poll_requested(); // frequency limited to a maximum rate/interval, GUI safe
    void duration_changed();
    void volume_changed();
    
    // Single atomic signal to send the whole block of data at once
    void track_changed();
    void playback_state_changed();
    void metadata_status_changed();

private slots:
    void handle_duration_changed();
    void handle_media_status_changed();
    void handle_playback_state_changed();

private:
    // Private constructor for the singleton pattern
    explicit playback_controller(QObject *parent = nullptr);
    ~playback_controller() override = default;

    QMediaPlayer m_media_player;
    QAudioOutput m_audio_output;

    // Protected internal status
    Types::Song m_current_track;
    metadata_load_status m_status = metadata_load_status::idle;
    
    // The core of synchronization: control of load versions
    uint64_t m_current_transaction_id = 0;

    // Throttle position_changed frequency for GUI by limiting to certain frequency
    
    // every time this timer ticks, a position_poll_requested signal
    // is triggered to avoid overloading the QML engine
    QTimer polling_position_timer;
    static constexpr quint16 polling_position_timer_interval = 250; // tick every...
};