#pragma once

#include <QObject>
#include <QUrl>
#include <QtTypes>

#include "mediatypes.hpp" 

namespace Types {
    struct Song;
}

// public interface for the audio_engine
class audio_engine
{

public: 
    enum class playback_state {
        paused,
        playing,
        stopped
    };

    virtual ~audio_engine() = default;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void unload() = 0;
    virtual void set_position(const quint64 position_ms) = 0;
    virtual void set_volume(quint8 volume_percent) = 0;
    virtual void load(const QUrl &source) = 0;
    virtual void prepare_next_track(const QUrl &source) = 0;
    virtual void undo_prepare_next_track() = 0;

    // no Types::Song below this point; the engine only sees sources
    void load(const Types::Song &song)
    {
        if (!song.is_valid()) return;
        load(song.source);
    }
    void prepare_next_track(const Types::Song &song) { prepare_next_track(song.source); }

    virtual const QUrl & current_track_source()       const = 0;
    virtual const QUrl & next_track_prepared_source() const = 0;
    virtual quint64     current_position_ms() const = 0;
    virtual quint8      current_volume()      const = 0;
    virtual playback_state get_playback_state() const = 0;
    virtual bool is_a_song_loaded() const = 0;
};

Q_DECLARE_INTERFACE(audio_engine, "com.noname.audio_controller")