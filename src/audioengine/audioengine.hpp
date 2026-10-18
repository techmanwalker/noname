#pragma once

#include "audioengine-in.hpp"

#include "audiointernalcontroller.hpp"

#include <QLoggingCategory>

enum class audio_backend;

/**
    @brief Playback controller proxy for whatever audio framework hides behind the scenes.

    @note Implemented this way so if the audio backend would ever change, the internal API
    stays mostly intact.

*/
/**
    @brief Playback controller proxy.
*/
class audio_engineLI : public QObject, public audio_engine
{
    Q_OBJECT
    Q_INTERFACES (audio_engine)
public:
    explicit audio_engineLI (QObject *parent, audio_backend backend);

    void play() override;
    void pause() override;
    void stop() override;
    void unload() override;
    void set_position(const quint64 position_ms) override;
    void set_volume(quint8 volume_percent) override;
    // keep audio_engine's Types::Song overloads visible through audio_engineLI
    // (declaring the overrides below would otherwise hide them)
    using audio_engine::load;
    using audio_engine::prepare_next_track;

    void load(const QUrl &source) override;
    void prepare_next_track(const QUrl &source) override;
    void undo_prepare_next_track() override;

    const QUrl & current_track_source()       const override;
    const QUrl & next_track_prepared_source() const override;
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

    QUrl m_current_track_source;
    QUrl m_prolly_next_track_source;

    double m_log_volume = 0;
    uint64_t m_current_transaction_id = 0;
};