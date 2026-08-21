#include "audiointernalcontroller.hpp"
#include "audiodecodeworker.hpp"
#include "l_audioengine.hpp" // logging categories

#include <QThread>

#include <soundio/soundio.h>

audio_internal_controller::audio_internal_controller(QObject *parent)
    : QObject(parent),
      m_audio_decoding_thread(new QThread(this)),
      m_decoder_worker(new audio_decode_worker())
{
    m_soundio = soundio_create();
    soundio_connect(m_soundio);
    soundio_flush_events(m_soundio);

    int default_out_device_index = soundio_default_output_device_index(m_soundio);
    m_device = soundio_get_output_device(m_soundio, default_out_device_index);

    m_outstream = soundio_outstream_create(m_device);
    m_outstream->format = SoundIoFormatFloat32NE; 
    m_outstream->sample_rate = m_decoder_worker->get_ring_buffer()->get_sample_rate();
    m_outstream->software_latency = 0.15;

    m_outstream->userdata = m_decoder_worker->get_ring_buffer();
    m_decoder_worker->get_ring_buffer()->rt_notify_target = this; // reachable from write_callback via userdata
    m_outstream->write_callback = write_callback;

    m_outstream->error_callback = error_callback;

    m_outstream->underflow_callback = underflow_callback;

    execute_soundio(soundio_outstream_open, m_outstream);
    execute_soundio(soundio_outstream_start, m_outstream);

    m_decoder_worker->get_ring_buffer()->is_paused.store(true);

    m_decoder_worker->moveToThread(m_audio_decoding_thread);

    connect(m_audio_decoding_thread, &QThread::started, 
            m_decoder_worker, &audio_decode_worker::start_decoding);

    connect(m_decoder_worker, &audio_decode_worker::seeked,
            this, &audio_internal_controller::seeked);
            
    m_audio_decoding_thread->start();
}

audio_internal_controller::~audio_internal_controller()
{
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();

    // stop the soundio callback first, the buffer ring must outlive it
    if (m_outstream) {
        soundio_outstream_destroy(m_outstream);
        m_outstream = nullptr;
    }

    // safely destroy
    QMetaObject::invokeMethod(m_decoder_worker, [worker = m_decoder_worker]() {
        delete worker;
    }, Qt::BlockingQueuedConnection);

    m_decoder_worker = nullptr;

    m_audio_decoding_thread->quit();
    m_audio_decoding_thread->wait();
    
    if (m_device) soundio_device_unref(m_device);
    if (m_soundio) soundio_destroy(m_soundio);
}

void audio_internal_controller::set_transport_paused(bool paused) {
    m_decoder_worker->get_ring_buffer()->is_paused.store(paused);
}

void audio_internal_controller::stop() {
    m_decoder_worker->get_ring_buffer()->is_paused.store(true);
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();
    
    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::clean, Qt::BlockingQueuedConnection);
    
    m_decoder_worker->get_ring_buffer()->reset_cancel();
}

void audio_internal_controller::load(const QString &file_path) {
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();
    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::load, Qt::BlockingQueuedConnection, file_path);
    m_decoder_worker->get_ring_buffer()->reset_cancel();
}

void audio_internal_controller::prepare_next_track(const Types::Song &song) {
    QMetaObject::invokeMethod(m_decoder_worker, [this, song]() {
        m_decoder_worker->queue_next_song(song);
    }, Qt::QueuedConnection);
}

void audio_internal_controller::undo_prepare_next_track() {
    QMetaObject::invokeMethod(m_decoder_worker, [this]() {
        m_decoder_worker->clear_queued_song();
    }, Qt::QueuedConnection);
}

void audio_internal_controller::set_position(uint64_t position_ms) {
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();
    m_decoder_worker->get_ring_buffer()->pending_seeks.fetch_add(1, std::memory_order_release);

    if (m_outstream) {
        int err = soundio_outstream_clear_buffer(m_outstream);
        if (err != 0 && err != SoundIoErrorIncompatibleBackend) {
            qCWarning(l_soundio) << "soundio_outstream_clear_buffer failed with exit code" << err << ":" << soundio_strerror(err);
        }
    }

    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::seek, Qt::QueuedConnection, position_ms);
}

void audio_internal_controller::set_volume_multiplier(float hw_multiplier) {
    m_decoder_worker->get_ring_buffer()->volume_multiplier.store(hw_multiplier, std::memory_order_relaxed);
}

bool audio_internal_controller::is_song_loaded() const {
    return m_decoder_worker->is_song_loaded();
}

uint64_t audio_internal_controller::current_position_ms() const {
    return static_cast<uint64_t>(m_decoder_worker->get_ring_buffer()->playback_position_ms());
}

bool audio_internal_controller::is_paused() const {
    return m_decoder_worker->get_ring_buffer()->is_paused.load();
}

bool audio_internal_controller::check_and_advance_boundary() {
    if (m_decoder_worker->get_ring_buffer()->check_for_boundary_and_advance()) {
        m_decoder_worker->get_ring_buffer()->frames_played.store(0, std::memory_order_relaxed);
        m_decoder_worker->get_ring_buffer()->playback_base_ms.store(0, std::memory_order_relaxed);
        return true;
    }
    return false;
}