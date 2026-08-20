#include "audioengine.hpp"
#include "audiointernalcontroller.hpp"
#include "mediatypes.hpp"

#include <QFuture>

#include <soundio/soundio.h>

// Logs are fundamental for this program not to blindly fall apart

Q_LOGGING_CATEGORY(l_audioengine, "noname.audioengine");

// Private constructor
audio_engine::audio_engine(QObject *parent)
    : QObject(parent),
      m_internal(std::make_unique<audio_internal_controller>(this))
{
    connect(m_internal.get(), &audio_internal_controller::seeked,
            this, &audio_engine::seek_finished);
            
    connect(this, &audio_engine::track_changed,
            this, &audio_engine::handle_track_changed);
}

void audio_engine::teardown ()
{
    m_internal.reset();
}

void audio_engine::play() {
    m_internal->set_transport_paused(false);
    emit playback_state_changed();
}

void audio_engine::pause() {
    m_internal->set_transport_paused(true);
    emit playback_state_changed();
}

void audio_engine::stop() {
    m_internal->stop();
    emit playback_state_changed();
}

void audio_engine::load(const Types::Song &song) {
    if (!song.is_valid()) return;

    m_current_transaction_id++;
    m_internal->load(song.source.toLocalFile());
    
    m_current_track = song;
    undo_prepare_next_track();

    emit track_changed();
}

void audio_engine::prepare_next_track(const Types::Song &song) {
    m_prolly_next_track = song;
    m_internal->prepare_next_track(song);
}

void audio_engine::undo_prepare_next_track() {
    m_prolly_next_track = Types::Song{};
    m_internal->undo_prepare_next_track();
}

void audio_engine::unload() {
    m_current_transaction_id++; 
    stop();
    m_current_track = Types::Song{};
    emit track_changed();
}

void audio_engine::set_position(const quint64 position_ms) {
    m_internal->set_position(position_ms);
}

void audio_engine::set_volume(quint8 volume_percent) {
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

quint8 audio_engine::current_volume() const {
    if (m_log_volume <= -std::numeric_limits<double>::infinity()) return 0;
    
    double amplitude = std::pow(10.0, m_log_volume / 20.0);
    double volume_percent = std::cbrt(amplitude) * 100.0;
    return static_cast<quint8>(qRound(volume_percent));
}

const Types::Song & audio_engine::current_track() const {
    return m_current_track;
}

const Types::Song & audio_engine::next_track_prepared() const {
    return m_prolly_next_track;
}

quint64 audio_engine::current_position_ms() const {
    if (!m_internal->is_song_loaded()) return 0;
    return m_internal->current_position_ms();
}

audio_engine::playback_state audio_engine::get_playback_state() const {
    using ps = audio_engine::playback_state;
    if (!m_internal->is_song_loaded()) return ps::stopped;
    return m_internal->is_paused() ? ps::stopped : ps::playing;
}

bool audio_engine::is_a_song_loaded() const {
    return m_current_track.is_valid();
}

void audio_engine::handle_track_changed() {
    emit duration_changed();
    emit seek_finished(); 
    emit playback_state_changed();
}

// To reactively trigger the changes signals
void audio_engine::process_track_boundary() {
    if (m_internal->check_and_advance_boundary()) {
        m_current_track = m_prolly_next_track;
        undo_prepare_next_track(); 
        
        emit track_changed();

        if (!m_current_track.is_valid()) {
            process_playlist_finished();
        }
    }
}

void audio_engine::process_playlist_finished() {
    emit queued_tracks_finished();
}