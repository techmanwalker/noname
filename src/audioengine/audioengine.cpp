#include "audioengine.hpp"
#include "bridgecontroller/internalcontroller.hpp"
#include "soundio/backends.hpp"

#include <QFuture>

#include <soundio/soundio.h>

// Logs are fundamental for this program not to blindly fall apart

// Private constructor
audio_engineLI::audio_engineLI(QObject *parent, audio_backend backend)
    : QObject(parent),
      m_internal(std::make_unique<audio_internal_controller>(this, backend))
{
    connect(m_internal.get(), &audio_internal_controller::seeked,
            this, &audio_engineLI::seek_finished);

    connect(m_internal.get(), &audio_internal_controller::load_failed,
            this, &audio_engineLI::load_failed);
            
    /*  Explicit Qt::QueuedConnection, kept on purpose: these signals are emitted by
        rt_event_pump on the main thread (never from the RT callback), so thread safety no
        longer depends on it. Queued still defers the slots out of the pump's drain loop,
        so UI work done in on_boundary_hit()/on_tape_ran_out() (and
        anything it triggers, like load()) can't re-enter the drain. */
    connect(m_internal.get(), &audio_internal_controller::boundary_hit,
            this, &audio_engineLI::on_boundary_hit, Qt::QueuedConnection);

    connect(m_internal.get(), &audio_internal_controller::tape_ran_out,
            this, &audio_engineLI::on_tape_ran_out, Qt::QueuedConnection);

    connect(this, &audio_engineLI::track_changed,
            this, &audio_engineLI::handle_track_changed);
}

void
audio_engineLI::teardown ()
{
    m_internal.reset();
}

void
audio_engineLI::play()
{
    m_internal->set_transport_paused(false);
    emit playback_state_changed();
}

void
audio_engineLI::pause()
{
    m_internal->set_transport_paused(true);
    emit playback_state_changed();
}

void
audio_engineLI::stop()
{
    m_internal->stop();
    emit playback_state_changed();
}

void
audio_engineLI::load(const QUrl &source)
{
    if (source.isEmpty()) return;

    m_current_transaction_id++;

    // The decoder decides between rewind and reopen: it knows what is actually open
    m_internal->load(source);

    m_track_A_source = source;
    undo_prepare_next_track();

    emit track_changed();
}

void
audio_engineLI::prepare_next_track(const QUrl &source)
{
    // Designated against an A that no longer exists: on_boundary_hit() just cleared B, and the
    // track_changed handler prepares the new one.
    if (catch_up_on_boundary_hit()) return;
    m_track_B_source = source;
    m_internal->prepare_B(source);
}

void
audio_engineLI::undo_prepare_next_track()
{
    if (catch_up_on_boundary_hit()) return;
    m_track_B_source.clear();
    m_internal->undo_prepare_B();
}

void
audio_engineLI::unload()
{
    m_current_transaction_id++; 
    stop();

    m_track_A_source.clear();
    emit track_changed();
}

void
audio_engineLI::relative_seek(qint64 step_ms)
{
    quint64 current_position = current_position_ms();
    quint64 step_ms_abs = std::abs(step_ms);

    // do not go before 0, avoid wrapping to huge value
    if (step_ms < 0 && current_position < step_ms_abs) {
        emit relative_seek_before_zero_performed(step_ms_abs, true);
        return;
    }

    set_position(current_position + step_ms);
}

void
audio_engineLI::set_position(const quint64 position_ms)
{
    // Computed against track A . If the boundary hit was overdue it has just been
    // consumed: that A is gone, drop the request, the UI re-derives from track_changed.
    if (catch_up_on_boundary_hit()) return;

    m_internal->set_position(position_ms);
}

void
audio_engineLI::set_volume(quint8 volume_percent)
{
    if (volume_percent > 100) volume_percent = 100;

    if (volume_percent == 0) {
        m_log_volume = -std::numeric_limits<double>::infinity();
    } else {
        double amplitude = std::pow(static_cast<double>(volume_percent) / 100.0, 3.0);
        m_log_volume = 20.0 * std::log10(amplitude);
    }

    float hw_multiplier = (volume_percent == 0) ? 0.0f : static_cast<float>(std::pow(10.0, m_log_volume / 20.0));
    m_internal->set_volume_multiplier(hw_multiplier);

    emit volume_changed();
}

quint8
audio_engineLI::current_volume() const
{
    if (m_log_volume <= -std::numeric_limits<double>::infinity()) return 0;
    
    double amplitude = std::pow(10.0, m_log_volume / 20.0);
    double volume_percent = std::cbrt(amplitude) * 100.0;
    return static_cast<quint8>(qRound(volume_percent));
}

const QUrl &
audio_engineLI::current_track_source() const
{
    return m_track_A_source;
}

const QUrl &
audio_engineLI::next_track_prepared_source() const
{
    return m_track_B_source;
}

quint64
audio_engineLI::current_position_ms() const
{
    if (!m_internal->is_song_loaded()) return 0;
    return m_internal->current_position_ms();
}

audio_engineLI::playback_state
audio_engineLI::get_playback_state() const
{
    using ps = audio_engineLI::playback_state;
    if (!m_internal->is_song_loaded()) return ps::stopped;
    return m_internal->is_paused() ? ps::stopped : ps::playing;
}

bool
audio_engineLI::is_a_song_loaded() const
{
    return !m_track_A_source.isEmpty();
}

void
audio_engineLI::handle_track_changed()
{
    emit duration_changed();
    emit seek_finished(); // to 0
    emit playback_state_changed();
}

// To reactively trigger the changes signals
void
audio_engineLI::on_boundary_hit()
{
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

        // Everything the ceremony and the track_changed handlers asked of the B slot is queued
        // ahead of this, so the decoder reads a settled slot
        m_internal->boundary_settled();

        // B failed to open: nothing behind A, the tape ran out
        if (m_track_A_source.isEmpty()) on_tape_ran_out();
    }
}

void
audio_engineLI::on_tape_ran_out()
{
    emit tape_ran_out();
}

bool
audio_engineLI::catch_up_on_boundary_hit ()
{
    if (!m_internal->boundary_hit_pending()) return false;
    on_boundary_hit();   // idempotent: the queued copy of this event becomes a no-op
    return true;
}