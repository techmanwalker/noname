#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "audioringbuffer.hpp"

#include "mediatypes.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

class audio_decode_worker : public QObject
{
    Q_OBJECT

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

public slots:
    // Queues the next file to be seamlessly opened upon EOF
    void queue_next_song(const Types::Song& next_song);
    void clear_queued_song();

private slots:
    // single work iteration, reprograms itself via m_decode_timer
    void decode_step ();

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
