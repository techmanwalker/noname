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
    // emitted from the decoder thread, delivered here
    connect(m_internal.get(), &audio_internal_controller::load_settled,
            this, &audio_engine_LI::on_load_settled, Qt::QueuedConnection);
    connect(m_internal.get(), &audio_internal_controller::seek_settled,
            this, &audio_engine_LI::on_seek_settled, Qt::QueuedConnection);
    connect(m_internal.get(), &audio_internal_controller::next_settled,
            this, &audio_engine_LI::on_next_settled, Qt::QueuedConnection);
    connect(m_internal.get(), &audio_internal_controller::stop_settled,
            this, &audio_engine_LI::on_stop_settled, Qt::QueuedConnection);

    /*  Explicit Qt::QueuedConnection, kept on purpose: these signals are emitted by
        rt_event_pump on the main thread (never from the RT callback), so thread safety no
        longer depends on it. Queued still defers the slots out of the pump's drain loop,
        so UI work done in on_boundary_hit()/on_tape_ran_out() (and
        anything it triggers, like load()) can't re-enter the drain. */
    connect(m_internal.get(), &audio_internal_controller::boundary_hit,
            this, &audio_engine_LI::on_boundary_hit, Qt::QueuedConnection);

    connect(m_internal.get(), &audio_internal_controller::tape_ran_out,
            this, &audio_engine_LI::on_tape_ran_out, Qt::QueuedConnection);

    connect(this, &audio_engine_LI::track_changed,
            this, &audio_engine_LI::handle_track_changed);
}

// Every public call lands here. The promise is made on the caller's thread, the body runs on the
// engine's, one call at a time: whichever reaches the engine last wins. The body must settle
// `call`: finish it, abandon() it, or hand it to a pending_calls group.
template <typename Body>
QFuture<void>
audio_engine_LI::dispatch (Body body)
{
    QPromise<void> promise;
    promise.start();
    QFuture<void> future = promise.future();

    auto run = [this, body = std::move(body)] (QPromise<void> call) mutable {
        if (m_internal) body(std::move(call));
        else            abandon(call);   // after teardown()
    };

    if (QThread::currentThread() == thread()) {
        run(std::move(promise));
    } else {
        // shared_ptr: the functor has to be copyable, a QPromise is not
        QMetaObject::invokeMethod(this, [run, call = std::make_shared<QPromise<void>>(std::move(promise))] () mutable {
            run(std::move(*call));
        }, Qt::QueuedConnection);
    }
    return future;
}

void
audio_engine_LI::teardown ()
{
    m_internal.reset();
}

QFuture<void>
audio_engine_LI::play()
{
    return dispatch([this] (QPromise<void> call) {
        m_internal->set_transport_paused(false);
        emit playback_state_changed();
        call.finish();
    });
}

QFuture<void>
audio_engine_LI::pause()
{
    return dispatch([this] (QPromise<void> call) {
        m_internal->set_transport_paused(true);
        emit playback_state_changed();
        call.finish();
    });
}

QFuture<void>
audio_engine_LI::stop()
{
    return dispatch([this] (QPromise<void> call) {
        m_transport_calls.add(std::move(call));   // settles in on_stop_settled()
        m_internal->stop();
        emit playback_state_changed();
    });
}

QFuture<void>
audio_engine_LI::load(const QUrl &source)
{
    if (source.isEmpty()) return {};   // a default-constructed future is cancelled

    return dispatch([this, source] (QPromise<void> call) {
        // before anything below can catch up on a hit of the old tape
        m_tape_calls.add(std::move(call));

        // The decoder decides between rewind and reopen: it knows what is actually open
        m_internal->load(source);

        m_track_A_source = source;
        undo_prepare_next_track();

        emit track_changed();
    });
}

QFuture<void>
audio_engine_LI::unload()
{
    return dispatch([this] (QPromise<void> call) {
        m_tape_calls.add(std::move(call));
        m_internal->unload();

        m_track_A_source.clear();
        emit track_changed();
        emit playback_state_changed();
    });
}

QFuture<void>
audio_engine_LI::relative_seek(qint64 step_ms)
{
    return dispatch([this, step_ms] (QPromise<void> call) {
        // Steps stack on the seek in flight: the clock has not moved yet
        const quint64 from        = m_seek_target.value_or(current_position_ms());
        const quint64 step_ms_abs = std::abs(step_ms);

        // do not go before 0, avoid wrapping to huge value
        if (step_ms < 0 && from < step_ms_abs) {
            emit relative_seek_before_zero_performed(step_ms_abs, true);
            call.finish();
            return;
        }

        start_seek(from + step_ms, std::move(call));
    });
}

QFuture<void>
audio_engine_LI::seek_to(const quint64 position_ms)
{
    return dispatch([this, position_ms] (QPromise<void> call) {
        start_seek(position_ms, std::move(call));
    });
}

void
audio_engine_LI::start_seek(quint64 position_ms, QPromise<void> call)
{
    // Computed against track A. If the boundary hit was overdue it has just been
    // consumed: that A is gone, drop the request, the UI re-derives from track_changed.
    if (catch_up_on_boundary_hit()) { abandon(call); return; }

    m_seek_target = position_ms;
    m_position_calls.add(std::move(call));
    m_internal->seek_to(position_ms);
}

QFuture<void>
audio_engine_LI::prepare_next_track(const QUrl &source)
{
    return dispatch([this, source] (QPromise<void> call) {
        // Designated against an A that no longer exists: on_boundary_hit() just cleared B, and the
        // track_changed handler prepares the new one.
        if (catch_up_on_boundary_hit()) { abandon(call); return; }

        m_track_B_source = source;
        m_next_calls.add(std::move(call));
        m_internal->prepare_B(source);
    });
}

QFuture<void>
audio_engine_LI::undo_prepare_next_track()
{
    return dispatch([this] (QPromise<void> call) {
        if (catch_up_on_boundary_hit()) { abandon(call); return; }

        m_track_B_source.clear();
        m_next_calls.add(std::move(call));
        m_internal->undo_prepare_B();
    });
}

QFuture<void>
audio_engine_LI::set_volume(quint8 volume_percent)
{
    return dispatch([this, volume_percent] (QPromise<void> call) {
        const quint8 percent = std::min<quint8>(volume_percent, 100);

        if (percent == 0) {
            m_log_volume = -std::numeric_limits<double>::infinity();
        } else {
            double amplitude = std::pow(static_cast<double>(percent) / 100.0, 3.0);
            m_log_volume = 20.0 * std::log10(amplitude);
        }

        float hw_multiplier = (percent == 0) ? 0.0f : static_cast<float>(std::pow(10.0, m_log_volume / 20.0));
        m_internal->set_volume_multiplier(hw_multiplier);

        emit volume_changed();
        call.finish();
    });
}

quint8
audio_engine_LI::current_volume() const
{
    if (m_log_volume <= -std::numeric_limits<double>::infinity()) return 0;

    double amplitude = std::pow(10.0, m_log_volume / 20.0);
    double volume_percent = std::cbrt(amplitude) * 100.0;
    return static_cast<quint8>(qRound(volume_percent));
}

const QUrl &
audio_engine_LI::current_track_source() const
{
    return m_track_A_source;
}

const QUrl &
audio_engine_LI::next_track_prepared_source() const
{
    return m_track_B_source;
}

quint64
audio_engine_LI::current_position_ms() const
{
    if (!m_internal->is_song_loaded()) return 0;
    return m_internal->current_position_ms();
}

audio_engine_LI::playback_state
audio_engine_LI::get_playback_state() const
{
    using ps = audio_engine_LI::playback_state;
    if (!m_internal->is_song_loaded()) return ps::stopped;
    return m_internal->is_paused() ? ps::stopped : ps::playing;
}

bool
audio_engine_LI::is_a_song_loaded() const
{
    return !m_track_A_source.isEmpty();
}

void
audio_engine_LI::handle_track_changed()
{
    m_seek_target.reset();
    emit duration_changed();
    emit seek_finished(); // to 0
    emit playback_state_changed();
}

void
audio_engine_LI::on_load_settled(bool ok)
{
    // track_changed went out before the decoder was ready: state and position are only true now
    if (m_tape_calls.last_one()) {
        emit playback_state_changed();
        emit seek_finished();
    }
    m_tape_calls.settle_oldest(ok);   // cancelled: superseded, or the open failed (load_failed says why)
}

void
audio_engine_LI::on_seek_settled(bool applied)
{
    if (m_position_calls.last_one()) {
        m_seek_target.reset();
        emit seek_finished();   // one per burst, whatever happened to the last one
    }
    m_position_calls.settle_oldest(applied);   // cancelled: superseded, stale epoch, or the seek failed
}

void
audio_engine_LI::on_next_settled()
{
    m_next_calls.settle_oldest(true);
}

void
audio_engine_LI::on_stop_settled()
{
    if (m_transport_calls.last_one()) emit playback_state_changed();   // the tape is gone only now
    m_transport_calls.settle_oldest(true);
}

// To reactively trigger the changes signals
void
audio_engine_LI::on_boundary_hit()
{
    // the load in flight replaces this tape and resets the ring: a hit on the old one is moot
    if (load_in_flight()) return;

    QUrl track_B_opened;
    if (m_internal->consume_boundary_hit(track_B_opened)) {
        // The decoder is the authority on what B really was, not what the engine prepared
        if (track_B_opened != m_track_B_source)
            qCWarning(l_audioengine) << "boundary hit: the decoder opened this file for B:" << track_B_opened
                                     << "but the engine prepared" << m_track_B_source;

        // B becomes A, and the B slot is empty again
        m_track_A_source = track_B_opened;
        undo_prepare_next_track();

        emit track_changed();
        emit ab_boundary_crossed();

        // Everything the ceremony and the track_changed handlers asked of the B slot is queued
        // ahead of this, so the decoder reads a settled slot
        m_internal->boundary_settled();

        // B failed to open: nothing behind A, the tape ran out
        if (m_track_A_source.isEmpty()) on_tape_ran_out();
    }
}

void
audio_engine_LI::on_tape_ran_out()
{
    if (load_in_flight()) return;   // same as on_boundary_hit
    emit tape_ran_out();
}

bool
audio_engine_LI::catch_up_on_boundary_hit ()
{
    if (load_in_flight() || !m_internal->boundary_hit_pending()) return false;
    on_boundary_hit();   // idempotent: the queued copy of this event becomes a no-op
    return true;
}
