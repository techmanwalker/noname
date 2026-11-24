#pragma once

#include <QFuture>
#include <QObject>
#include <QPromise>
#include <QString>
#include <QUrl>

#include <atomic>
#include <memory>
#include <optional>

#include "bridgecontroller/pendingcalls.hpp"
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

    // ---- what audio_engine_LI forwards to: every future ends settled or cancelled ----

    // state
    const QUrl &current_track_source ()       const { return m_track_A_source; }
    const QUrl &next_track_prepared_source () const { return m_track_B_source; }
    bool        is_a_track_loaded ()          const { return !m_track_A_source.isEmpty(); }
    quint8      current_volume ()             const;
    quint64     current_position_ms ()        const;   // 0 while the decoder holds no song
    bool        is_song_loaded ()             const;
    bool        is_paused ()                  const;

    // transport
    QFuture<void> play();
    QFuture<void> pause();
    QFuture<void> stop();

    // tape (A)
    QFuture<void> load(const QUrl &source);
    QFuture<void> unload();

    // next (B)
    QFuture<void> prepare_next_track(const QUrl &source);
    QFuture<void> undo_prepare_next_track();

    // position
    QFuture<void> seek_to(quint64 position_ms);
    QFuture<void> relative_seek(qint64 step_ms);

    // volume
    QFuture<void> set_volume(quint8 volume_percent);

signals:
    // Re-exported by audio_engine_LI
    void load_failed(const QUrl &source, const QString &reason);
    void seek_finished();
    void duration_changed();
    void track_changed();
    void media_loaded_changed();   // A went from empty to loaded or back (set_track_A)
    void tape_ran_out();           // A's samples ended, no B; never while a load is in flight
    void playback_state_changed();
    void volume_changed();
    void relative_seek_before_zero_performed(quint64 stepback_ms, bool play_afterwards);
    void ab_boundary_crossed();

private:
    // Emitted from the decoder thread, delivered (queued) to the slots below
    Q_SIGNAL void load_settled(bool ok);        // a load()/unload() job ended. true = it ran (load: the decoder holds the file); false = superseded, or the open failed
    Q_SIGNAL void seek_settled(bool applied);   // a seek_to() job ended. false = superseded, stale epoch, or the seek failed
    Q_SIGNAL void next_settled();               // a prepare_B()/undo_prepare_B() job ran

private slots:
    void on_boundary_hit();   // rt_event_pump: A's samples ended, B follows. Also runs early via catch_up_on_boundary_hit()
    void on_tape_ran_out();   // rt_event_pump: A's samples ended, no B
    void on_load_settled(bool ok);
    void on_seek_settled(bool applied);
    void on_next_settled();
    void handle_track_changed();

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

    std::atomic<uint64_t> m_load_generation { 0 };   // load() calls so far: a job older than this is superseded

    template <typename Job> void queue_tape_job (Job job);
    void clear_device_buffer ();

    std::atomic<uint64_t> m_seek_generation { 0 };   // seek_to() calls so far: a job older than this is superseded

    // ---- call-level state: nothing outside this class can reach it ----
    QUrl m_track_A_source;   // A (empty: nothing loaded). Writer: set_track_A()
    QUrl m_track_B_source;   // B as prepared by prepare_next_track()/undo_prepare_next_track(); on_boundary_hit() asks the decoder which file it really opened

    double m_log_volume = 0; // Writer: set_volume()

    // Calls the decoder thread still owes a settlement for, one queue per group
    pending_calls m_tape_calls;        // load(), unload()
    pending_calls m_position_calls;    // seek_to(), relative_seek()
    pending_calls m_next_calls;        // prepare_next_track(), undo_prepare_next_track()

    // Where the seeks in flight end up: the clock has not moved yet.
    // Writers: start_seek(), on_seek_settled(), handle_track_changed()
    std::optional<quint64> m_seek_target;

    template <typename Body> QFuture<void> dispatch (Body body);
    void set_track_A (const QUrl &source);   // the one writer of A, emits media_loaded_changed
    void start_seek (quint64 position_ms, QPromise<void> call);
    void start_unload (QPromise<void> call);
    bool refuse_without_track (QPromise<void> &call);   // transport, position and B all need an A

    bool catch_up_on_boundary_hit ();
    bool load_in_flight () const { return !m_tape_calls.empty(); }

    // ---- decoder-thread plumbing, only the call-level API above drives it ----
    void set_transport_paused(bool paused);
    void queue_load(const QUrl &source);
    void queue_unload();
    void queue_seek(uint64_t position_ms);
    void set_volume_multiplier(float hw_multiplier);

    // B: the file the decoder opens as soon as it has read A's file to the end; B's samples
    // then follow A's in the ring. Absent = the tape runs out where A's samples end.
    void prepare_B(const QUrl &source);
    void undo_prepare_B();

    // true = the RT thread hit the boundary since the last call (consumed: B is the new A).
    // track_B_opened is the file the decoder really opened for B, empty if opening it failed.
    bool consume_boundary_hit(QUrl &track_B_opened);
    bool boundary_hit_pending () const;   // hit by the RT thread, not consumed yet
    void boundary_settled();
};

#include "logging/prettyerrors.tpp" // IWYU pragma: keep
