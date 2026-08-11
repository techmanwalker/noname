#include "audioengine.hpp"
#include "mediatypes.hpp"

#include <QFuture>

#include <soundio/soundio.h>

// Logs are fundamental for this program not to blindly fall apart

Q_LOGGING_CATEGORY(l_audioengine, "noname.audioengine");

// Meyers singleton implementation
audio_engine &
audio_engine::instance()
{
    static audio_engine s_instance;
    return s_instance;
}

// Private constructor
audio_engine::audio_engine(QObject *parent)
    : QObject(parent)
{
    // initialize libsoundio
    m_soundio = soundio_create();
    soundio_connect(m_soundio);
    soundio_flush_events(m_soundio);

    // get default sound card
    int default_out_device_index = soundio_default_output_device_index(m_soundio);
    m_device = soundio_get_output_device(m_soundio, default_out_device_index);

    // create output stream
    m_outstream = soundio_outstream_create(m_device);

    // use our format
    // TODO: unhardcode the sample rate and format for bit-perfect playback
    m_outstream->format = SoundIoFormatFloat32NE; 
    m_outstream->sample_rate = m_decoder_worker->get_ring_buffer()->get_sample_rate();

    // avoid playing position drift
    m_outstream->software_latency = 0.15;

    // connect hardware with our ring buffer
    m_outstream->userdata = m_decoder_worker->get_ring_buffer();
    m_outstream->write_callback = write_callback;

    // open stream an boot, starting paused
    // additionally log error codes to catch bugs
    execute_soundio(soundio_outstream_open, m_outstream);
    execute_soundio(soundio_outstream_start, m_outstream);
    m_decoder_worker->get_ring_buffer()->is_paused.store(true);

    // fire up ffmpeg decoding thread
    m_decoder_worker->moveToThread(m_audio_decoding_thread);

    connect(m_audio_decoding_thread, &QThread::started, 
            m_decoder_worker, &audio_decode_worker::start_decoding);

    connect(m_decoder_worker, &audio_decode_worker::seeked,
            this, &audio_engine::seek_finished);
            
    m_audio_decoding_thread->start();

    connect(this, &audio_engine::track_changed,
            this, &audio_engine::handle_track_changed);
}

audio_engine::~audio_engine()
{
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();

    m_audio_decoding_thread->quit();
    m_audio_decoding_thread->wait();
    
    // clean
    if (m_outstream) soundio_outstream_destroy(m_outstream);
    if (m_device) soundio_device_unref(m_device);
    if (m_soundio) soundio_destroy(m_soundio);

    // safely done in main thread
    delete m_audio_decoding_thread;
}

void
audio_engine::set_transport_paused(bool paused)
{
    m_decoder_worker->get_ring_buffer()->is_paused.store(paused);

    emit playback_state_changed();
}



void
audio_engine::play()
{
    set_transport_paused(false); // play means "unpause"

    emit playback_state_changed();
}

void
audio_engine::pause()
{
    set_transport_paused(true);

    emit playback_state_changed();
}

void
audio_engine::stop()
{
    m_decoder_worker->get_ring_buffer()->is_paused.store(true);

    // Unblock before cleaning
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();
    
    // clean ring buffer
    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::clean, Qt::BlockingQueuedConnection);
    
    m_decoder_worker->get_ring_buffer()->reset_cancel();

    emit playback_state_changed();
}

void
audio_engine::load (const Types::Song &song) // song IS the metadata, no need to async wait
{
    if (!song.is_valid()) return;

    m_current_transaction_id++;

    // Unblock the decoder thread so it can return to its event loop

    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();

    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::load, Qt::BlockingQueuedConnection, song.source.toLocalFile());

    // Re-arm for normal operation
    m_decoder_worker->get_ring_buffer()->reset_cancel();

    // after loading

    m_current_track = song;

    // the queue was just wiped, ui needs to decide what song to preload next
    undo_prepare_next_track();

    emit track_changed();
}

void audio_engine::prepare_next_track(const Types::Song &song) {
    m_prolly_next_track = song;
    QMetaObject::invokeMethod(m_decoder_worker, [this, song]() {
        m_decoder_worker->queue_next_song(song);
    }, Qt::QueuedConnection);
}

void 
audio_engine::undo_prepare_next_track() 
{
    // Clear the local front-end tracker
    m_prolly_next_track = Types::Song{};
    
    // Asynchronously clear the backend queue
    QMetaObject::invokeMethod(m_decoder_worker, [this]() {
        m_decoder_worker->clear_queued_song();
    }, Qt::QueuedConnection);
}

void
audio_engine::unload()
{
    m_current_transaction_id++; // invalidate pending loads

    stop();
    
    m_current_track = Types::Song{};

    emit track_changed();
}

void
audio_engine::set_position(const quint64 position_ms)
{
    // 1. Immediately wake up the decoder thread if it's asleep in push_blocking()
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();

    // 2. Flag the RT thread to stop popping old audio and feed zeroes
    m_decoder_worker->get_ring_buffer()->pending_seeks.fetch_add(1, std::memory_order_release);

    // 3. Clear the OS hardware buffer so any already-popped audio is dropped
    if (m_outstream) {
        int err = soundio_outstream_clear_buffer(m_outstream);
        
        // Pipewire/PulseAudio backends do not support buffer clearing and return 11.
        // We explicitly ignore this specific error to prevent console flooding.
        if (err != 0 && err != SoundIoErrorIncompatibleBackend) {
            qCWarning(l_soundio) << "soundio_outstream_clear_buffer failed with exit code" << err << ":" << soundio_strerror(err);
        }
    }

    // 4. Queue the seek in the decoder thread
    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::seek, Qt::QueuedConnection, static_cast<uint64_t>(position_ms));
}

void
audio_engine::set_volume(quint8 volume_percent)
{
    if (volume_percent > 100) {
        volume_percent = 100;
    }

    if (volume_percent == 0) {
        m_log_volume = -std::numeric_limits<double>::infinity();
    } else {
        double amplitude = std::pow(static_cast<double>(volume_percent) / 100.0, 3.0);
        m_log_volume = 20.0 * std::log10(amplitude);
    }

    float hw_multiplier = (volume_percent == 0) ? 0.0f : static_cast<float>(std::pow(10.0, m_log_volume / 20.0));
    
    // Transmitir el volumen al búfer DSP para su procesamiento en tiempo real
    m_decoder_worker->get_ring_buffer()->volume_multiplier.store(hw_multiplier, std::memory_order_relaxed);

    emit volume_changed();
}

quint8
audio_engine::current_volume() const
{
    // catch the sentinel value first
    if (m_log_volume <= -std::numeric_limits<double>::infinity()) {
        return 0;
    }

    // reverse the dB calculation back to the amplitude multiplier
    double amplitude = std::pow(10.0, m_log_volume / 20.0);
    
    // reverse the cubic curve mapping back to the linear 0-100 ui scale
    double volume_percent = std::cbrt(amplitude) * 100.0;

    return static_cast<quint8>(qRound(volume_percent));
}

const Types::Song &
audio_engine::current_track() const
{
    return m_current_track;
}

const Types::Song &
audio_engine::next_track_prepared() const
{
    return m_prolly_next_track;
}

quint64
audio_engine::current_position_ms() const
{
    if (!m_decoder_worker->is_song_loaded()) return 0;

    // who better than the decoder itself
    return static_cast<quint64>(m_decoder_worker->get_ring_buffer()->playback_position_ms());
}

audio_engine::playback_state
audio_engine::get_playback_state() const
{
    using ps = audio_engine::playback_state;

    if (!m_decoder_worker->is_song_loaded()) return ps::stopped;

    if (!m_decoder_worker->get_ring_buffer()->is_paused.load()) {
        return ps::playing;
    } else {
        return ps::stopped;
    }
}

bool
audio_engine::is_a_song_loaded() const {
    return m_current_track.is_valid();
}

/*
audio_engine::media_status
audio_engine::get_media_status() const
{
    // todo: sync with sndio... or probably not
}*/

void
audio_engine::handle_track_changed ()
{
    emit duration_changed();
    emit seek_finished(); // to 0:00
    emit playback_state_changed();
}

// To reactively trigger the changes signals
void
audio_engine::process_track_boundary()
{
    if (m_decoder_worker->get_ring_buffer()->check_for_boundary_and_advance()) {

        m_current_track = m_prolly_next_track;
        
        undo_prepare_next_track(); // nothing is preloaded yet :)

        // Reset local track playback position to 0
        m_decoder_worker->get_ring_buffer()->frames_played.store(0, std::memory_order_relaxed);
        m_decoder_worker->get_ring_buffer()->playback_base_ms.store(0, std::memory_order_relaxed);

        // No need to peek for upcoming boundaries.
        // The atomic next_boundary_frame was already set to UINT64_MAX by write_callback.

        emit track_changed();

        // If the 'next' track was empty/invalid, we hit the end of the line
        if (!m_current_track.is_valid()) {
            process_playlist_finished();
        }
    }
}

void
audio_engine::process_playlist_finished()
{
    emit queued_tracks_finished();
}