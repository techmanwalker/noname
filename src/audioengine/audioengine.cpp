#include "audioengine.hpp"
#include "bridgecontroller/internalcontroller.hpp"
#include "soundio/backends.hpp"

#include <QFuture>
#include <QThread>

#include <memory>
#include <optional>

#include <soundio/soundio.h>

// Logs are fundamental for this program not to blindly fall apart

// Private constructor
audio_engine_LI::audio_engine_LI(QObject *parent, audio_backend backend)
    : QObject(parent),
      m_internal(std::make_unique<audio_internal_controller>(this, backend))
{
    // The controller owns the state and decides when it changes: its signals are ours
    using ic = audio_internal_controller;
    connect(m_internal.get(), &ic::load_failed,                         this, &audio_engine_LI::load_failed);
    connect(m_internal.get(), &ic::seek_finished,                       this, &audio_engine_LI::seek_finished);
    connect(m_internal.get(), &ic::duration_changed,                    this, &audio_engine_LI::duration_changed);
    connect(m_internal.get(), &ic::track_changed,                       this, &audio_engine_LI::track_changed);
    connect(m_internal.get(), &ic::media_loaded_changed,                this, &audio_engine_LI::media_loaded_changed);
    connect(m_internal.get(), &ic::tape_ran_out,                        this, &audio_engine_LI::tape_ran_out);
    connect(m_internal.get(), &ic::playback_state_changed,              this, &audio_engine_LI::playback_state_changed);
    connect(m_internal.get(), &ic::volume_changed,                      this, &audio_engine_LI::volume_changed);
    connect(m_internal.get(), &ic::relative_seek_before_zero_performed, this, &audio_engine_LI::relative_seek_before_zero_performed);
    connect(m_internal.get(), &ic::ab_boundary_crossed,                 this, &audio_engine_LI::ab_boundary_crossed);
}

namespace {

// After teardown() there is no controller: the call ends cancelled, like any refused call
template <typename... Params, typename... Args>
QFuture<void>
forward (const std::unique_ptr<audio_internal_controller> &internal,
         QFuture<void> (audio_internal_controller::*call) (Params...), Args &&...args)
{
    if (internal) return (internal.get()->*call)(std::forward<Args>(args)...);

    QPromise<void> promise;
    promise.start();
    abandon(promise);
    return promise.future();
}

}

void audio_engine_LI::teardown () { m_internal.reset(); }

// state
const QUrl & audio_engine_LI::current_track_source ()       const { return m_internal->current_track_source(); }
const QUrl & audio_engine_LI::next_track_prepared_source () const { return m_internal->next_track_prepared_source(); }
quint64      audio_engine_LI::current_position_ms ()        const { return m_internal->current_position_ms(); }
quint8       audio_engine_LI::current_volume ()             const { return m_internal->current_volume(); }
bool         audio_engine_LI::is_a_track_loaded ()          const { return m_internal->is_a_track_loaded(); }

audio_engine_LI::playback_state
audio_engine_LI::get_playback_state() const
{
    using ps = audio_engine_LI::playback_state;
    if (!m_internal->is_song_loaded()) return ps::stopped;
    return m_internal->is_paused() ? ps::paused : ps::playing;
}

// transport
QFuture<void> audio_engine_LI::play  () { return forward(m_internal, &audio_internal_controller::play); }
QFuture<void> audio_engine_LI::pause () { return forward(m_internal, &audio_internal_controller::pause); }
QFuture<void> audio_engine_LI::stop  () { return forward(m_internal, &audio_internal_controller::stop); }

// tape (a)
QFuture<void> audio_engine_LI::load (const QUrl &source) { return forward(m_internal, &audio_internal_controller::load, source); }
QFuture<void> audio_engine_LI::unload () { return forward(m_internal, &audio_internal_controller::unload); }

// next (b)
QFuture<void> audio_engine_LI::prepare_next_track (const QUrl &source) { return forward(m_internal, &audio_internal_controller::prepare_next_track, source); }
QFuture<void> audio_engine_LI::undo_prepare_next_track () { return forward(m_internal, &audio_internal_controller::undo_prepare_next_track); }

// position
QFuture<void> audio_engine_LI::seek_to (const quint64 position_ms) { return forward(m_internal, &audio_internal_controller::seek_to, position_ms); }
QFuture<void> audio_engine_LI::relative_seek (qint64 step_ms)      { return forward(m_internal, &audio_internal_controller::relative_seek, step_ms); }

// volume
QFuture<void> audio_engine_LI::set_volume (quint8 volume_percent) { return forward(m_internal, &audio_internal_controller::set_volume, volume_percent); }
