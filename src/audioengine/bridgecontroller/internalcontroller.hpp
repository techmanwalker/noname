#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <memory>

#include "decode/worker.hpp"
#include "ringbuffer/ringbuffer.hpp"

#include "realtime/rteventpump.hpp"

enum class audio_backend;

// log simplifier, avoids super verbose function calls
#define execute_soundio(func, ...) \
    audio_internal_controller::log_soundio_internal(#func, func, __VA_ARGS__)

/**
    @brief Core processor managing the hardware boundaries and decoding loops.
*/
class audio_internal_controller : public QObject
{
    Q_OBJECT
public:
    explicit audio_internal_controller(QObject *parent, audio_backend backend);
    ~audio_internal_controller() override;

    template<typename Func, typename... Args>
    static int log_soundio_internal(const char* func_name, Func soundio_func, Args&&... args);

    void set_transport_paused(bool paused);
    void stop();
    void load(const QUrl &source);

    // B: the file the decoder opens as soon as it has read A's file to the end; B's samples
    // then follow A's in the ring. Absent = the tape runs out where A's samples end.
    void prepare_B(const QUrl &source);
    void undo_prepare_B();

    void set_position(uint64_t position_ms);
    void set_volume_multiplier(float hw_multiplier);

    bool is_song_loaded() const;
    uint64_t current_position_ms() const;
    bool is_paused() const;
    // true = the RT thread hit the boundary since the last call (consumed: B is the new A).
    // track_B_opened is the file the decoder really opened for B, empty if opening it failed.
    bool consume_boundary_hit(QUrl &track_B_opened);
    bool boundary_hit_pending () const;   // hit by the RT thread, not consumed yet
    void boundary_settled();

signals:
    void seeked();
    void load_failed(const QUrl &source, const QString &reason);   // a file could not be opened (load, or B at the gapless handoff)
    void boundary_hit();   // emitted by rt_event_pump (this thread): A's samples ended, B follows
    void tape_ran_out();   // emitted by rt_event_pump (this thread): A's samples ended, no B

private:
    // Owned here, so this is the only place that can reach the ring's control surface. The decoder
    // holds the producer end, the libsoundio callbacks the rt_end (through the stream's userdata).
    // Declared first, so it is destroyed after everything that points into it.
    std::unique_ptr<audio_ring_buffer> m_ring;

    QThread *m_audio_decoding_thread = nullptr;
    audio_decode_worker *m_decoder_worker = nullptr;

    struct SoundIo *m_soundio = nullptr;
    struct SoundIoDevice *m_device = nullptr;
    struct SoundIoOutStream *m_outstream = nullptr;

    rt_event_pump *m_rt_pump = nullptr;
};

#include "logging/prettyerrors.tpp" // IWYU pragma: keep