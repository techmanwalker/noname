#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace Types {
    struct Song;
}

class PlayerPresenter {

public:
    // direct mirror of audio_controller::playback_state
    enum class PlaybackState {
        paused,
        playing,
        stopped
    };

    virtual ~PlayerPresenter () = default;

    virtual QString title() const = 0;
    virtual QString artist() const = 0;
    virtual QString album() const = 0;
    virtual QUrl   cover() const = 0;
    virtual quint64 duration_ms() const = 0;
    virtual quint64 position_ms() const = 0;
    virtual quint8 volume() const = 0;
    virtual PlaybackState playbackState() const = 0;
    virtual bool isMediaLoaded() const = 0;

    virtual void setPosition_ms(quint64 position) = 0;
    virtual void setVolume (quint8 volume) = 0;

    virtual void play()  const = 0;
    virtual void pause() const = 0;
    virtual void stop()  const = 0;
    virtual void next()  const = 0;
    virtual void prev()  const = 0;

    virtual void load(Types::Song &song) const = 0;

    virtual void notify_slider_pressed_change (bool pressed) = 0;
    virtual void saveVolume () const = 0;
};

Q_DECLARE_INTERFACE(PlayerPresenter, "com.noname.PlayerPresenter")