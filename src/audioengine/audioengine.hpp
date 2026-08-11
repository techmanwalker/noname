#pragma once

#include "mediatypes.hpp"

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
#include <queue>

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

struct track_boundary {
    uint64_t absolute_frame_start; 
    Types::Song song_metadata;
};

class audio_ring_buffer
{
    friend class audio_decode_worker;

    const uint64_t sample_rate; // by default

    size_t capacity = sample_rate * 10 * 2; // ~5s of stereo audio

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

    // Tracks total frames pushed by the decoder since the engine started
    std::atomic_uint64_t absolute_frames_decoded{0};
    
    // Tracks total frames consumed by libsoundio
    std::atomic_uint64_t absolute_frames_played{0};

    // Tracks ongoing seek operations to mute the hardware loop
    std::atomic<int> pending_seeks{0};

    // Governed by a perceptual volume control that rescales logarithmic values back for soundio
    std::atomic<float> volume_multiplier{1.0f};
    
    // Handshake sequence to safely reset head/tail without clobbering
    std::atomic_bool flush_request{false};
    std::atomic_bool flush_ack{false};

    std::mutex boundary_mutex;
    std::queue<track_boundary> upcoming_boundaries;

    // Target frame count for the next upcoming track boundary (UINT64_MAX when none)
    std::atomic_uint64_t next_boundary_frame{UINT64_MAX};

    std::optional<Types::Song> check_and_pop_boundary();
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

    // Queues the next file to be seamlessly opened upon EOF
    void queue_next_song(const Types::Song& next_song);
    void clear_queued_song();

signals:
    void song_load_failed (QString &err);

    void seeked();

private:

    friend class audio_engine;

    int convert_to_float (AVFrame *frame, float **output);

    audio_ring_buffer m_ring_buffer;

    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    SwrContext *swr_ctx = nullptr;
    
    QTimer *m_decode_timer = nullptr;

    std::atomic_bool m_song_loaded {false};

    // decode and seek are mutually exclusive
    std::mutex m_decoder_mutex;

    Types::Song m_queued_next_song;
    bool m_has_queued_song{false};
    std::mutex m_queue_mutex;

    // Closes FFmpeg contexts without clearing the ring buffer
    void close_ffmpeg_contexts(); 
    
    // Opens a new file and prepares SwrContext (essentially the FFmpeg parts of your current load() method)
    bool open_file_internal(const QString& file_path);

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

    // Disable copy and reassignment to guarantee single instance
    audio_engine(const audio_engine&) = delete;
    audio_engine &operator=(const audio_engine&) = delete;

    // Meyers singleton instance for global access within C++
    static audio_engine &instance();

    // Friendlier error messages, return the errcode itself
    template<typename Func, typename... Args>
    static int log_soundio_internal(const char* func_name, Func soundio_func, Args&&... args);

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

    void prepare_next_track(const Types::Song &song);
    void undo_prepare_next_track();

    // Safe getters for current data status
    const Types::Song & current_track()       const;
    const Types::Song & next_track_prepared() const;
    quint64     current_position_ms() const;
    quint8      current_volume()      const; // volume from 0 to 100
    playback_state get_playback_state() const;
    bool is_a_song_loaded() const;

    // Reactively wait for EOF or track switches
    void process_track_boundary();
    void process_playlist_finished();

signals:
    void seek_finished();
    void duration_changed();
    
    // Single atomic signal to send the whole block of data at once
    void track_changed();
    void queued_tracks_finished();
    void playback_state_changed();
    void volume_changed();

public slots:

    void handle_track_changed (); // mostly reemit signals

private:
    // Private constructor for the singleton pattern
    explicit audio_engine(QObject *parent = nullptr);
    ~audio_engine() override;

    QThread *m_audio_decoding_thread = new QThread();
    audio_decode_worker *m_decoder_worker = new audio_decode_worker();

    struct SoundIo *m_soundio = nullptr;
    struct SoundIoDevice *m_device = nullptr;
    struct SoundIoOutStream *m_outstream = nullptr;

    // Protected internal status
    Types::Song m_current_track;

    Types::Song m_prolly_next_track;

    double m_log_volume = 0;
    
    // The core of synchronization: control of load versions
    uint64_t m_current_transaction_id = 0;
};

#include "prettyerrors.tpp" // IWYU pragma: keep