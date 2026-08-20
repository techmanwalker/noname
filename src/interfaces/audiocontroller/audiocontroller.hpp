#pragma once

#include <QObject>
#include <QtTypes>

namespace Types {
    struct Song;
}

// public interface for the audio_engine
class audio_controller {

public: 
    enum class playback_state {
        paused,
        playing,
        stopped
    };

    virtual ~audio_controller() = default;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void unload() = 0;
    virtual void set_position(const quint64 position_ms) = 0;
    virtual void set_volume(quint8 volume_percent) = 0;
    virtual void load(const Types::Song &song) = 0;
    virtual void prepare_next_track(const Types::Song &song) = 0;
    virtual void undo_prepare_next_track() = 0;

    virtual const Types::Song & current_track()       const = 0;
    virtual const Types::Song & next_track_prepared() const = 0;
    virtual quint64     current_position_ms() const = 0;
    virtual quint8      current_volume()      const = 0;
    virtual playback_state get_playback_state() const = 0;
    virtual bool is_a_song_loaded() const = 0;
};

Q_DECLARE_INTERFACE(audio_controller, "com.noname.audio_controller")