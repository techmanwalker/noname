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
            this, &audio_engine::position_changed);

    m_eof_poll_timer = new QTimer(this);
    m_eof_poll_timer->setInterval(100); // 10 Hz

    // poll for eof tracking
    connect(m_eof_poll_timer, &QTimer::timeout, this, [this]() {
        // 1. Check for gapless track transitions
        if (auto next_song = m_decoder_worker->get_ring_buffer()->check_and_pop_boundary()) {
            
            m_current_track = next_song.value();
            
            // Reset the local track playback time to 0 while keeping the hardware stream running
            m_decoder_worker->get_ring_buffer()->frames_played.store(0, std::memory_order_relaxed);
            m_decoder_worker->get_ring_buffer()->playback_base_ms.store(0, std::memory_order_relaxed);
            
            // Let the UI know the song has officially changed
            emit track_changed(); 
        }

        // 2. Normal EOF check
        if (m_decoder_worker->get_ring_buffer()->eof_played.exchange(false)) {
            // Handle end of playlist / transport stop, or unloaded next song
            emit queued_tracks_finished();
        }
    });
            
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

    if (paused) {
        m_eof_poll_timer->stop();
    } else {
        m_eof_poll_timer->start();
    }

    emit playback_state_changed();
}



void
audio_engine::play()
{
    set_transport_paused(false); // play means "unpause"
}

void
audio_engine::pause()
{
    set_transport_paused(true);
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

    emit track_changed();
}

void audio_engine::prepare_next_track(const Types::Song &song) {
    m_prolly_next_track = song;
    QMetaObject::invokeMethod(m_decoder_worker, [this, song]() {
        m_decoder_worker->queue_next_song(song);
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
    // Force upper limit
    if (volume_percent > 100) {
        volume_percent = 100;
    }

    // Convert the 0..100 scale to 0.0f..1.0f for QAudioOutput
    float volume_float = static_cast<float>(volume_percent) / 100.0f;

    // todo: set it on libsndio

    // Emit signal
    emit volume_changed();
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

quint8
audio_engine::current_volume() const
{
    // Fetch 0-1 volume value
    // todo: implement with sndio

    // Scale up to 0-100
    // return static_cast<quint8>(qRound(volume_float * 100.0f));
    return 100;
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
    emit position_changed();
    emit playback_state_changed();
}