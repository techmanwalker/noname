#include "bridgecontroller/internalcontroller.hpp"
#include "decode/worker.hpp"
#include "logging/l_audioengine.hpp" // logging categories
#include "soundio/backends.hpp"

#include <QThread>

#include <soundio/soundio.h>

namespace {

constexpr SoundIoFormat
to_soundio (sample_format f) noexcept
{
    switch (f) {
    case sample_format::u8:    return SoundIoFormatU8;
    case sample_format::s8:    return SoundIoFormatS8;
    case sample_format::s16le: return SoundIoFormatS16LE;
    case sample_format::s16be: return SoundIoFormatS16BE;
    case sample_format::s24le: return SoundIoFormatS24LE;
    case sample_format::s24be: return SoundIoFormatS24BE;
    case sample_format::s32le: return SoundIoFormatS32LE;
    case sample_format::s32be: return SoundIoFormatS32BE;
    case sample_format::f32le: return SoundIoFormatFloat32LE;
    case sample_format::f32be: return SoundIoFormatFloat32BE;
    case sample_format::f64le: return SoundIoFormatFloat64LE;
    case sample_format::f64be: return SoundIoFormatFloat64BE;
    case sample_format::mulaw:
    case sample_format::alaw:  break; // libsoundio has no companded formats
    }
    return SoundIoFormatInvalid;
}

// Shared-server mode: the device decides, we adapt. Only formats with an RT kernel are
// candidates, best first (float: no precision loss from the decoder, nothing to dither).
stream_format
negotiate_stream_format (SoundIoDevice *dev, int preferred_rate)
{
    stream_format fmt { f32ne, 48000, 2 }; // what we try when the device tells us nothing
    if (!dev) return fmt;

    for (const sample_format candidate : { f32ne, s32ne, s16ne, f64ne })
        if (soundio_device_supports_format(dev, to_soundio(candidate))) { fmt.sample = candidate; break; }

    if (const int rate = soundio_device_nearest_sample_rate(dev, preferred_rate); rate > 0)
        fmt.rate = uint32_t(rate);

    return fmt;
}

} // namespace

audio_internal_controller::audio_internal_controller(QObject *parent, audio_backend backend)
    : QObject(parent),
      m_audio_decoding_thread(new QThread(this))
{
    m_soundio = soundio_create();

    qCDebug (l_audioengine) << "Target backend: " << map::backend_to_string_view(backend);

    // Prioritize PulseAudio (seamlessly handled by PipeWire)
    int err = soundio_connect_backend(m_soundio, map::backend_to_soundio_backend(backend));

    const char* early_backend_name = soundio_backend_name(m_soundio->current_backend);

    if (err) {
        // it will crash anyway
        qCCritical (l_audioengine) << "Error connecting soundio: " << soundio_strerror(err) << "; ended up with " << early_backend_name;
        std::exit(EXIT_FAILURE);
    } else {
        qCDebug(l_audioengine) << "SoundIO connected backend: " << early_backend_name;
    }

    soundio_flush_events(m_soundio);

    const char* backend_name = soundio_backend_name(m_soundio->current_backend);
    qCDebug(l_audioengine) << "SoundIO connected to audio backend:" << backend_name;

    int default_out_device_index = soundio_default_output_device_index(m_soundio);

    m_device = soundio_get_output_device(m_soundio, default_out_device_index);

    // the ring/decoder are built FROM the negotiated format, so negotiation comes first
    const stream_format negotiated = negotiate_stream_format(m_device, 48000);
    m_decoder_worker = new audio_decode_worker(negotiated);

    m_outstream = soundio_outstream_create(m_device);
    m_outstream->format      = to_soundio(negotiated.sample);
    m_outstream->sample_rate = int(negotiated.rate);

    m_outstream->software_latency = 0.15;

    m_outstream->layout = *soundio_channel_layout_get_default(negotiated.channels);

    auto ring = m_decoder_worker->get_ring_buffer();

    m_outstream->userdata = ring;

    m_rt_pump = new rt_event_pump(ring->rt_events, this);
    connect(m_rt_pump, &rt_event_pump::tape_ran_out,      this, &audio_internal_controller::tape_ran_out);
    connect(m_rt_pump, &rt_event_pump::boundary_hit, this, &audio_internal_controller::boundary_hit);

    m_outstream->write_callback = write_callback;

    m_outstream->error_callback = error_callback;

    m_outstream->underflow_callback = underflow_callback;

    if (execute_soundio(soundio_outstream_open, m_outstream) == 0) {
        qCInfo(l_audioengine) << "outstream negotiated:"
            << "backend" << soundio_backend_name(m_soundio->current_backend)
            << "rate" << m_outstream->sample_rate
            << "channels" << m_outstream->layout.channel_count
            << "device channels" << m_device->current_layout.channel_count;

        if (m_outstream->layout_error)
            qCWarning(l_audioengine) << "layout not honoured exactly:" << soundio_strerror(m_outstream->layout_error);

        if (m_outstream->format != to_soundio(negotiated.sample)
            || uint32_t(m_outstream->sample_rate) != negotiated.rate
            || m_outstream->layout.channel_count != negotiated.channels)
            qCCritical(l_audioengine) << "backend changed the stream format; ring and device disagree";
    }

    execute_soundio(soundio_outstream_start, m_outstream);

    ring->is_paused.store(true);

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

    // the callback can no longer post. drop the pump BEFORE the ring (and the eventfd
    // it listens on) is destroyed together with the worker.
    delete m_rt_pump;
    m_rt_pump = nullptr;

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

void
audio_internal_controller::set_transport_paused(bool paused)
{
    m_decoder_worker->get_ring_buffer()->is_paused.store(paused);
}

void
audio_internal_controller::stop()
{
    m_decoder_worker->get_ring_buffer()->is_paused.store(true);
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();
    
    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::clean, Qt::BlockingQueuedConnection);
    
    m_decoder_worker->get_ring_buffer()->reset_cancel();
}

void
audio_internal_controller::load(const QUrl &source)
{
    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();
    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::load, Qt::BlockingQueuedConnection, source);
    m_decoder_worker->get_ring_buffer()->reset_cancel();
}

void
audio_internal_controller::prepare_B(const QUrl &source)
{
    QMetaObject::invokeMethod(m_decoder_worker, [this, source]() {
        m_decoder_worker->prepare_B(source);
    }, Qt::QueuedConnection);
}

void
audio_internal_controller::undo_prepare_B()
{
    QMetaObject::invokeMethod(m_decoder_worker, [this]() {
        m_decoder_worker->undo_prepare_B();
    }, Qt::QueuedConnection);
}

void
audio_internal_controller::set_position(uint64_t position_ms)
{
    // The epoch this seek is issued under: the decoder refuses it if a ceremony ran since
    const uint64_t epoch = m_decoder_worker->get_ring_buffer()->boundary_snapshot().epoch;

    m_decoder_worker->get_ring_buffer()->cancel_blocking_push();
    m_decoder_worker->get_ring_buffer()->pending_seeks.fetch_add(1, std::memory_order_release);

    if (m_outstream) {
        int err = soundio_outstream_clear_buffer(m_outstream);
        if (err != 0 && err != SoundIoErrorIncompatibleBackend) {
            qCWarning(l_soundio) << "soundio_outstream_clear_buffer failed with exit code" << err << ":" << soundio_strerror(err);
        }
    }

    QMetaObject::invokeMethod(m_decoder_worker, &audio_decode_worker::seek, Qt::QueuedConnection, position_ms, epoch);
}

void
audio_internal_controller::set_volume_multiplier(float hw_multiplier)
{
    m_decoder_worker->get_ring_buffer()->volume_multiplier.store(hw_multiplier, std::memory_order_relaxed);
}

bool
audio_internal_controller::is_song_loaded() const
{
    return m_decoder_worker->is_song_loaded();
}

uint64_t
audio_internal_controller::current_position_ms() const
{
    return static_cast<uint64_t>(m_decoder_worker->get_ring_buffer()->playback_position_ms());
}

bool
audio_internal_controller::is_paused() const
{
    return m_decoder_worker->get_ring_buffer()->is_paused.load();
}

bool
audio_internal_controller::consume_boundary_hit (QUrl &track_B_opened)
{
    const QUrl opened = m_decoder_worker->track_B_opened();   // read before consuming
    if (!m_decoder_worker->get_ring_buffer()->consume_boundary_hit()) return false;
    track_B_opened = opened;
    return true;
}

bool
audio_internal_controller::boundary_hit_pending () const
{
    using phase = audio_ring_buffer::boundary_phase;
    return m_decoder_worker->get_ring_buffer()->boundary_snapshot().phase == phase::hit;
}