#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

#include "ringbuffer/ringbuffer.hpp"
#include "decode/avscratch.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

class audio_decode_worker : public QObject
{
    Q_OBJECT

public:
    explicit audio_decode_worker (stream_format out_format, QObject *parent = nullptr);
    ~audio_decode_worker() override;

    // The file the decoder really opened for B when it reached the end of A's file
    // (empty if opening it failed). Thread safe.
    QUrl track_B_opened () const;

public slots:

    audio_ring_buffer* get_ring_buffer();

    // call if needed to load some other song or audio stream
    void load (const QUrl &source);

    // ffmpeg decoding timer, triggers decode_step() repeatedly
    void start_decoding ();

    // trigger to flush and start reading from some other timestamp
    // epoch: boundary_snapshot().epoch when the request was issued (see audioringbuffer.hpp)
    void seek (uint64_t position_ms, uint64_t epoch);

    // clean the buffer
    void clean ();

    bool is_song_loaded () const;

public slots:
    // B: the file the decoder opens once it has read A's file to the end, so the ring runs on without a gap
    void prepare_B(const QUrl &source);
    void undo_prepare_B();

    // The engine consumed a boundary hit and already queued its undo_prepare_B / prepare_B for it:
    // the B slot is settled. Resolves a deferred end of file (see m_end_deferred).
    void boundary_settled ();

private slots:
    // single work iteration, reprograms itself via m_decode_timer
    void decode_step ();

signals:
    void song_load_failed (QUrl source, QString err);   // the file that failed to open, and why

    void seeked();

private:

    friend class audio_engine;

    // false = the push was cancelled (seek/stop/load): the caller must bail out
    bool push_frame (AVFrame *frame);

    // true when the decoder's output already IS the ring format: no swr, no per-frame malloc
    bool           m_passthrough = false;
    AVSampleFormat m_out_av_fmt  = AV_SAMPLE_FMT_NONE;

    // The file fmt_ctx has open right now (empty when none). The decoder reads ahead of what is
    // being heard, so this is the file that belongs to A most of the time, and the file that
    // belongs to B once the decoder has read A's file to the end but A is still playing out.
    // To know whether a file is already open, ask this, never the engine's A.
    QUrl m_open_source;

    // Body shared by seek() and load()'s same-file rewind. false = nothing was repositioned
    // (seek failed or, when an epoch is given, the request was stale).
    bool rewind_in_place (uint64_t position_ms, std::optional<uint64_t> epoch = std::nullopt);

    // Common tail of every reposition. Caller holds m_decoder_mutex and the quiesce.
    bool seek_and_reset (const audio_ring_buffer::rt_quiesce &quiesce, uint64_t position_ms);

    // Boundary armed (A still audible, the decoder already reading B's file): reopen A's file at
    // position_ms, B is prepared again
    bool reopen_A_at (const audio_ring_buffer::rt_quiesce &quiesce, uint64_t position_ms);

    // The decoder has ONE file open (m_open_source). These exist for the armed window, when the
    // open file (B's) is not the one being heard (A's).
    QUrl m_track_A_source;   // A's file, remembered when the decoder moved on to B's; valid only while armed (decoder thread)
    QUrl m_track_B_opened;   // B's file as the decoder really opened it (guarded by m_B_mutex)

    audio_ring_buffer m_ring_buffer;

    // Reused packet/frame (no per-tick alloc). Reachable only through av_scratch::lease.
    av_scratch m_scratch;

    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    SwrContext *swr_ctx = nullptr;
    
    QTimer *m_decode_timer = nullptr;

    std::atomic_bool m_song_loaded {false};

    // decode and seek are mutually exclusive
    std::mutex m_decoder_mutex;

    // B's file as prepared: opened (and consumed) when the decoder reaches the end of A's file.
    // Guarded by m_B_mutex.
    QUrl m_track_B_source;
    bool m_has_B{false};
    mutable std::mutex m_B_mutex;

    // Closes FFmpeg contexts without clearing the ring buffer
    void close_ffmpeg_contexts(); 

    // A's file is fully in the ring: hand over to B or end the tape. Returns the next timer interval.
    int on_end_of_A ();

    // The open file ended while the boundary before it (A/B) was still drawn, so its own end cannot
    // be drawn yet: the ring holds ONE boundary. Decoder thread, under m_decoder_mutex. Cleared by
    // close_ffmpeg_contexts() and seek_and_reset() too (decoding moved on). Resolved by boundary_settled().
    bool m_end_deferred = false;

    // Nothing decodable from here on: close the contexts and unload. By default the tape ends too, so
    // the ring plays out what it holds. Pass true when nothing was ever played (failed load: the ring
    // was just reset, there is no tape to end).
    void abandon_decoding (bool do_not_mark_end_of_tape = false);
    
    // Opens a new file and prepares SwrContext (essentially the FFmpeg parts of your current load() method)
    bool open_file_internal(const QUrl &source);

    int audio_stream_index = -1;
};
