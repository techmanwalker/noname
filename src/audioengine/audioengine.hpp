#pragma once

#include "mediatypes.hpp"

#include "audiocontroller.hpp"

#include "audiointernalcontroller.hpp"

#include <QLoggingCategory>

/**
    @brief Playback controller proxy for whatever audio framework hides behind the scenes.

    @note Implemented this way so if the audio backend would ever change, the internal API
    stays mostly intact.

*/
/**
    @brief Playback controller proxy.
*/
class audio_engine : public QObject, public audio_controller
{
    Q_OBJECT
    Q_INTERFACES (audio_controller)
public:
    explicit audio_engine (QObject *parent);

    void play() override;
    void pause() override;
    void stop() override;
    void unload() override;
    void set_position(const quint64 position_ms) override;
    void set_volume(quint8 volume_percent) override;
    void load(const Types::Song &song) override;
    void prepare_next_track(const Types::Song &song) override;
    void undo_prepare_next_track() override;

    const Types::Song & current_track()       const override;
    const Types::Song & next_track_prepared() const override;
    quint64     current_position_ms() const override;
    quint8      current_volume()      const override;
    playback_state get_playback_state() const override;
    bool is_a_song_loaded() const override;

    void process_track_boundary();
    void process_playlist_finished();

    void teardown (); // called by its governor &Q*Application::abouToQuit to safely destroy the impl

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
    std::unique_ptr<audio_internal_controller> m_internal;

    Types::Song m_current_track;
    Types::Song m_prolly_next_track;

    double m_log_volume = 0;
    uint64_t m_current_transaction_id = 0;
};