#pragma once

#include <QObject>
#include <QString>

#include "audiodecodeworker.hpp"

#include "mediatypes.hpp"

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
    explicit audio_internal_controller(QObject *parent = nullptr);
    ~audio_internal_controller() override;

    template<typename Func, typename... Args>
    static int log_soundio_internal(const char* func_name, Func soundio_func, Args&&... args);

    void set_transport_paused(bool paused);
    void stop();
    void load(const QString &file_path);
    void prepare_next_track(const Types::Song &song);
    void undo_prepare_next_track();
    void set_position(uint64_t position_ms);
    void set_volume_multiplier(float hw_multiplier);

    bool is_song_loaded() const;
    uint64_t current_position_ms() const;
    bool is_paused() const;
    bool check_and_advance_boundary();

signals:
    void seeked();

private:
    QThread *m_audio_decoding_thread = nullptr;
    audio_decode_worker *m_decoder_worker = nullptr;

    struct SoundIo *m_soundio = nullptr;
    struct SoundIoDevice *m_device = nullptr;
    struct SoundIoOutStream *m_outstream = nullptr;
};