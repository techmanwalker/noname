#pragma once

#include "mediatypes.hpp"
#include "playqueue.hpp"

#include <QLoggingCategory>
#include <QObject>
#include <QString>
#include <QStringLiteral>
#include <QThread>
#include <QTimer>

#include <atomic>
#include <cstddef>

#include <cstdint>
#include <qloggingcategory.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <soundio/soundio.h>

Q_DECLARE_LOGGING_CATEGORY(l_audioengine) // errors in audioengine itself
Q_DECLARE_LOGGING_CATEGORY(l_soundio) // soundio specific errors
Q_DECLARE_LOGGING_CATEGORY(l_ffmpeg) // errors in ffmpeg decoding

// log simplifier, avoids super verbose function calls
#define execute_soundio(func, ...) \
    audio_engine::log_soundio_internal(#func, func, __VA_ARGS__)

// sndio
void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);

class audio_ring_buffer
{
    friend class audio_decode_worker;

    const uint64_t sample_rate; // by default

    size_t capacity = sample_rate * 5 * 2; // ~5s of stereo audio

    // not deque so it does not move
    std::vector<float> buffer;

    std::atomic<size_t> head{0}; // where FFmpeg writes
    std::atomic<size_t> tail{0}; // where libsoundio reads

    // synchronization for a blocking push, fundamental to avoid skim decoding
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic_bool m_cancelled{false};

    // internal push to ring buffer
    bool __push_unlocked (const float* data, size_t count);

public:

    audio_ring_buffer (std::optional<size_t> requested_sample_rate = std::nullopt);

    // called by the ffmpeg thread to write to the output sound pipe
    // blocking push - guaranteed to succeed unless cancelled
    bool push_blocking(const float* data, size_t count);

    // cancel an in-flight blocking push (call before seek/reset)
    void cancel_blocking_push();

    // re-arm after cancel
    void reset_cancel();

    // called by libsoundio write_callback
    size_t pop (float* dest, size_t count);

    bool is_full () const;
    size_t writable_size() const;

    // implemented here to read during write_callback
    std::atomic_bool is_paused {false};

    // track eof
    std::atomic_bool eof_decoded {false}; // set by decoder when it flushes the last frame
    std::atomic_bool eof_played {false};  // set by libsoundio when the buffer drains

    
    // Playback time tracking

    // last manually seeked/loaded timestamp, where frames_played = 0 corresponds to
    std::atomic_uint64_t playback_base_ms {0};

    // delay, in frames, after the playback_base_ms time, never reset by write_callback
    std::atomic_uint64_t frames_played {0}; 

    // Current playback position in ms, derived from the above.
    uint64_t playback_position_ms() const;



    // Stream data
    uint64_t get_sample_rate () const;
};

class audio_decode_worker : public QObject
{
    Q_OBJECT
    // I don't know if the decoding workhorse should live here or on audio_engine

public:
    explicit audio_decode_worker (QObject *parent = nullptr);
    ~audio_decode_worker() override;

public slots:

    audio_ring_buffer* get_ring_buffer();

    // call if needed to load some other song or audio stream
    void load (const QString &file_path);

    // ffmpeg decoding timer, triggers decode_step() repeatedly
    void start_decoding ();

    // trigger to flush and start reading from some other timestamp
    void seek (uint64_t position_ms);

    // clean the buffer
    void clean ();

    bool is_song_loaded () const;

private slots:
    // single work iteration, reprograms itself via m_decode_timer
    void decode_step ();

signals:
    void song_load_failed (QString &err);

    void seeked();

private:

    int convert_to_float (AVFrame *frame, float **output);

    audio_ring_buffer m_ring_buffer;

    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    SwrContext *swr_ctx = nullptr;
    
    QTimer *m_decode_timer = nullptr;

    std::atomic_bool m_song_loaded {false};

    // decode and seek are mutually exclusive
    std::mutex m_decoder_mutex;

    int audio_stream_index = -1;
};

/**
    @brief Playback controller proxy for whatever audio framework hides behind the scenes.

    @note Implemented this way so if the audio backend would ever change, the internal API
    stays mostly intact.

*/
class audio_engine : public QObject
{
    Q_OBJECT
public:
    enum class playback_state {
        paused,
        playing,
        stopped
    };

    /*

    enum class media_status {
        no_media,
        loading_media,
        loaded_media,
        stalled_media,
        buffering_media,
        buffered_media,
        end_of_media,
        invalid_media
    };
    */


    // Disable copy and reassignment to guarantee single instance
    audio_engine(const audio_engine&) = delete;
    audio_engine &operator=(const audio_engine&) = delete;

    // Meyers singleton instance for global access within C++
    static audio_engine &instance();

    // Friendlier error messages, return the errcode itself
    template<typename Func, typename... Args>
    static int log_soundio_internal(const char* func_name, Func soundio_func, Args&&... args) {
        int errcode = soundio_func(std::forward<Args>(args)...);

        if (errcode != 0) {
            qCWarning(l_soundio) 
                << func_name 
                << " failed with exit code " 
                << errcode 
                << ": " 
                << soundio_strerror(errcode);
        }

        return errcode; // nesting support
    }

    void play();
    void pause();
    void stop();
    void unload();
    void set_position(const quint64 position_ms);
    void set_volume(quint8 volume_percent);

    // play(), pause() and stop() can't drift from each other
    void set_transport_paused(bool paused);

    // Load process is synchronous on its call, but asynchronous on its resolution
    void load(const Types::Song &song);

    // Safe getters for current data status
    Types::Song current_track()       const;
    quint64     current_position_ms() const;
    quint8      current_volume()      const; // volume from 0 to 100
    playback_state get_playback_state() const;
    // media_status get_media_status()     const;

signals:
    void position_changed();
    void duration_changed();
    void volume_changed();
    
    // Single atomic signal to send the whole block of data at once
    void track_changed();
    void playback_state_changed();
    void song_finished();

    // Reverse signal from QML down to the controller
    void r_duration_slider_pressed_changed(bool pressed);

private slots:
    void handle_duration_slider_pressed_changed(bool pressed);
    void handle_playhead_changed(bool play_afterwards = false); // triggered by a switch_to or click in QML

    void handle_track_changed (); // mostly reemit signals

    void handle_song_finished (); // opportunity for gapless playback

private:
    // Private constructor for the singleton pattern
    explicit audio_engine(QObject *parent = nullptr);
    ~audio_engine() override;

    QThread *m_audio_decoding_thread = new QThread();
    audio_decode_worker *m_decoder_worker = new audio_decode_worker();

    struct SoundIo *m_soundio = nullptr;
    struct SoundIoDevice *m_device = nullptr;
    struct SoundIoOutStream *m_outstream = nullptr;

    // Polls the ring buffer's playback clock while actively playing. The
    // decoder never needs to know or care about UI update cadence.
    QTimer *m_position_poll_timer = nullptr;

    // Protected internal status
    Types::Song m_current_track;

    playback_state playback_state_when_last_slider_drag_started;
    
    // The core of synchronization: control of load versions
    uint64_t m_current_transaction_id = 0;

    PlayQueue &queue = PlayQueue::instance();
};