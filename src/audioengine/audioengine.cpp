#include "audioengine.hpp"
#include "audiointernalcontroller.hpp"
#include "soundiobackends.hpp"

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
            
    /*  Explicit Qt::QueuedConnection, kept on purpose: these signals are emitted by
        rt_event_pump on the main thread (never from the RT callback), so thread safety no
        longer depends on it. Queued still defers the slots out of the pump's drain loop,
        so UI work done in process_track_boundary()/process_playlist_finished() (and
        anything it triggers, like load()) can't re-enter the drain. */
    connect(m_internal.get(), &audio_internal_controller::track_boundary_crossed,
            this, &audio_engineLI::process_track_boundary, Qt::QueuedConnection);

    connect(m_internal.get(), &audio_internal_controller::playlist_finished,
            this, &audio_engineLI::process_playlist_finished, Qt::QueuedConnection);

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

    m_current_track_source = source;
    undo_prepare_next_track();

    emit track_changed();
}

void
audio_engineLI::prepare_next_track(const QUrl &source)
{
    if (settle_boundary()) return;
    m_prolly_next_track_source = source;
    m_internal->prepare_next_track(source);
}

void
audio_engineLI::undo_prepare_next_track()
{
    if (settle_boundary()) return;
    m_prolly_next_track_source.clear();
    m_internal->undo_prepare_next_track();
}

void
audio_engineLI::unload()
{
    m_current_transaction_id++; 
    stop();

    m_current_track_source.clear();
    emit track_changed();
}

void
audio_engineLI::set_position(const quint64 position_ms)
{
    // Computed on the track the UI was showing. If the ceremony was overdue it has just run,
    // and that track is gone: drop it, the UI re-derives from track_changed.
    if (settle_boundary()) return;
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
    return m_current_track_source;
}

const QUrl &
audio_engineLI::next_track_prepared_source() const
{
    return m_prolly_next_track_source;
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
    return !m_current_track_source.isEmpty();
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
audio_engineLI::process_track_boundary()
{
    QUrl crossed_into;
    if (m_internal->check_and_advance_boundary(crossed_into)) {
        // The decoder is the authority on what became audible
        if (crossed_into != m_prolly_next_track_source)
            qCWarning(l_audioengine) << "boundary crossed into" << crossed_into
                                     << "but the engine expected" << m_prolly_next_track_source;

        m_current_track_source = crossed_into;
        undo_prepare_next_track();

        emit track_changed();

        if (m_current_track_source.isEmpty()) {
            process_playlist_finished();
        }
    }
}

void
audio_engineLI::process_playlist_finished()
{
    emit queued_tracks_finished();
}

bool
audio_engineLI::settle_boundary ()
{
    if (!m_internal->boundary_reached()) return false;
    process_track_boundary();   // idempotent: the queued copy of this event becomes a no-op
    return true;
}