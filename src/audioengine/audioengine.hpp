#pragma once

#include "mediatypes.hpp"

#include "audiointernalcontroller.hpp"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(l_audioengine) // errors in audioengine itself
Q_DECLARE_LOGGING_CATEGORY(l_soundio) // soundio specific errors
Q_DECLARE_LOGGING_CATEGORY(l_ffmpeg) // errors in ffmpeg decoding

/**
    @brief Playback controller proxy for whatever audio framework hides behind the scenes.

    @note Implemented this way so if the audio backend would ever change, the internal API
    stays mostly intact.

*/
/**
    @brief Playback controller proxy.
*/
class audio_engine : public QObject
{
    Q_OBJECT
public:
    enum class playback_state {
        paused,
        playing,
        stopped
    };

    audio_engine(const audio_engine&) = delete;
    audio_engine &operator=(const audio_engine&) = delete;

    static audio_engine &instance();

    void play();
    void pause();
    void stop();
    void unload();
    void set_position(const quint64 position_ms);
    void set_volume(quint8 volume_percent);
    void load(const Types::Song &song);
    void prepare_next_track(const Types::Song &song);
    void undo_prepare_next_track();

    const Types::Song & current_track()       const;
    const Types::Song & next_track_prepared() const;
    quint64     current_position_ms() const;
    quint8      current_volume()      const;
    playback_state get_playback_state() const;
    bool is_a_song_loaded() const;

    void process_track_boundary();
    void process_playlist_finished();

signals:
    void seek_finished();
    void duration_changed();
    void track_changed();
    void queued_tracks_finished();
    void playback_state_changed();
    void volume_changed();

public slots:
    void handle_track_changed();

private:
    explicit audio_engine(QObject *parent = nullptr);
    ~audio_engine() override = default;

    std::unique_ptr<audio_internal_controller> m_internal;

    Types::Song m_current_track;
    Types::Song m_prolly_next_track;

    double m_log_volume = 0;
    uint64_t m_current_transaction_id = 0;
};



#include "prettyerrors.tpp" // IWYU pragma: keep