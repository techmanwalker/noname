#pragma once

#include "virtual/audioengine.hpp"

#include "bridgecontroller/internalcontroller.hpp"
#include "bridgecontroller/pendingcalls.hpp"

#include <QLoggingCategory>
#include <QPromise>
#include <QFuture>

#include <optional>

enum class audio_backend;

/**
    @brief Playback controller proxy for whatever audio framework hides behind the scenes.

    @note Implemented this way so if the audio backend would ever change, the internal API
    stays mostly intact.

*/
/**
    @brief Playback controller proxy.
*/
class audio_engine_LI : public QObject, public audio_engine
{
    Q_OBJECT
    Q_INTERFACES (audio_engine)
public:
    explicit audio_engine_LI (QObject *parent, audio_backend backend);

    // state
    const QUrl &   current_track_source()       const override;
    const QUrl &   next_track_prepared_source() const override;
    quint64        current_position_ms() const override;
    quint8         current_volume()      const override;
    playback_state get_playback_state() const override;
    bool           is_a_song_loaded() const override;

    // transport
    QFuture<void> play() override;
    QFuture<void> pause() override;
    QFuture<void> stop() override;

    // tape (a)
    // todo: load needs to return false when the load fails
    QFuture<void> load(const QUrl &source) override;
    QFuture<void> unload() override;

    // next (b)
    QFuture<void> prepare_next_track(const QUrl &source) override;
    QFuture<void> undo_prepare_next_track() override;

    // position
    QFuture<void> seek_to(const quint64 position_ms) override;
    QFuture<void> relative_seek (qint64 step_ms) override;

    // volume
    QFuture<void> set_volume(quint8 volume_percent) override;

    // The RT thread hit the boundary: B becomes A. Also runs early via catch_up_on_boundary_hit().
    void on_boundary_hit();
    void on_tape_ran_out();   // the boundary was hit with no B behind it

    void teardown (); // called by its governor &Q*Application::abouToQuit to safely destroy the impl

signals:
    void seek_finished();
    void load_failed(const QUrl &source, const QString &reason);   // forwarded from the decoder
    void duration_changed();
    void track_changed();
    void tape_ran_out();
    void playback_state_changed();
    void volume_changed();
    void relative_seek_before_zero_performed(quint64 stepback_ms, bool play_afterwards);

    // exposure
    void ab_boundary_crossed ();

public slots:
    void handle_track_changed();
    void on_load_settled(bool ok);
    void on_seek_settled(bool applied);
    void on_next_settled();
    void on_stop_settled();

private:
    std::unique_ptr<audio_internal_controller> m_internal;

    QUrl m_track_A_source;   // A (empty: nothing loaded)
    QUrl m_track_B_source;   // B as the engine prepared it; on_boundary_hit() asks the decoder which file it really opened for B

    double m_log_volume = 0;

    // Calls the decoder thread still owes a settlement for, one queue per group
    pending_calls m_transport_calls;   // stop()
    pending_calls m_tape_calls;        // load(), unload()
    pending_calls m_position_calls;    // seek_to(), relative_seek()
    pending_calls m_next_calls;        // prepare_next_track(), undo_prepare_next_track()

    // Where the seeks in flight end up: the clock has not moved yet
    std::optional<quint64> m_seek_target;

    template <typename Body> QFuture<void> dispatch (Body body);
    void start_seek (quint64 position_ms, QPromise<void> call);

    bool catch_up_on_boundary_hit ();
    bool load_in_flight () const { return !m_tape_calls.empty(); }
};
