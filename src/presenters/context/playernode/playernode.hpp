#pragma once

#include "audiocontroller.hpp"
#include "lyricsprojector.hpp"
#include "playerpresenter.hpp"

#include <QLoggingCategory>
#include <QObject>
#include <QTimer>
#include <QString>
#include <QUrl>

#include <QtQmlIntegration/qqmlintegration.h>

#include <atomic>
#include <memory>

// forward declarations
class QQmlEngine;
class QJSEngine;

class LyricsManifest;
class PlayQueue;

namespace Types {
    class Song;
}

/**
    @class PlayerNode
    @brief Declarative and reactive representation of the playback state for the user interface.

    This class acts as the view model within the application architecture,
    serving as the single source of truth for visual components (QML) regarding the current 
    state of the audio engine. Its fundamental purpose is to structure and expose playback data 
    efficiently, completely decoupling business logic and audio processing from the interface.

    @section Data flow (detachment)
    The information flow is strictly compartmentalized on the C++ axis:
    - Input (Visual Update): A low-level audio controller (e.g., %PlaybackController) 
    manipulates the audio hardware/backend and updates the data of this class through its public slots 
    (@ref updateMetadata, @ref updatePosition).
    - Output (User Requests): When the user interacts with the interface (such as dragging a progress 
    bar or changing the volume), the class does not modify the internal state directly; instead, it emits 
    request signals (@ref requestSeek, @ref requestVolumeChange) so that the C++ controller validates and executes 
    the action on the audio engine.

    @note This class is designed as a Singleton managed by the QML engine (@c QML_SINGLETON) and exposes 
    read-only properties for static metadata, ensuring that the interface cannot corrupt 
    the current media state directly.
*/
class PlayerNode : public QObject, public PlayerPresenter
{
    Q_OBJECT
    Q_INTERFACES(PlayerPresenter)
    // its qml proxy is located in its interfaces subfolder and needs audio_controller
    // and lyricsprojector already injected

public:
    explicit PlayerNode(
        QObject *parent = nullptr,
        std::shared_ptr<audio_controller> controller = nullptr,
        std::shared_ptr<LyricsProjector> lyricsproj = nullptr
    );
    
    using PlayerPresenter::PlaybackState;
    Q_ENUM(PlaybackState)

    // Q_PROPERTY defines the magic variables that QML can read and listen
    Q_PROPERTY(QString title       READ title       NOTIFY titleChanged)
    Q_PROPERTY(QString artist      READ artist      NOTIFY artistChanged)
    Q_PROPERTY(QString album       READ album       NOTIFY albumChanged)
    Q_PROPERTY(QUrl    cover       READ cover       NOTIFY coverChanged)
    Q_PROPERTY(quint64 duration_ms READ duration_ms NOTIFY durationChanged)
    Q_PROPERTY(quint64 position_ms READ position_ms WRITE setPosition_ms    NOTIFY positionChanged)
    Q_PROPERTY(quint8  volume      READ volume      WRITE setVolume         NOTIFY volumeChanged)

    Q_PROPERTY(bool isMediaLoaded READ isMediaLoaded NOTIFY mediaLoadedChanged)

    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)

    // Getters
    QString title() const override;
    QString artist() const override;
    QString album() const override;
    QUrl   cover() const override;
    quint64 duration_ms() const override;
    quint64 position_ms() const override;
    quint8 volume() const override;
    PlaybackState playbackState() const override;
    bool isMediaLoaded() const override;

    // Setters (normally called from C++ logic when time or song changes)
    void setPosition_ms(quint64 position) override;
    void setVolume (quint8 volume) override;

    // Call forwardings
    void load(Types::Song &song) const override;

    // Playback controls
    Q_INVOKABLE void play()  const override;
    Q_INVOKABLE void pause() const override;
    Q_INVOKABLE void stop()  const override;
    Q_INVOKABLE void next()  const override;
    Q_INVOKABLE void prev()  const override;

    // to stop the poll timer while scrubbing
    Q_INVOKABLE void notify_slider_pressed_change (bool pressed) override;

    // save volume level to disk on demand
    Q_INVOKABLE void saveVolume () const override;
    
signals:
    // Needed signals for QML to be reactive
    void titleChanged();
    void artistChanged();
    void albumChanged();
    void coverChanged();
    void durationChanged();
    void positionChanged();
    void volumeChanged();
    void playbackStateChanged();
    void mediaLoadedChanged();
    void sliderPressedChanged();

public slots:
    // Formally expose as a metadata block receiver
    void handleTrackChanged();
    void handlePlaybackStateChanged();
    void handleSliderPressedChanged(); // diff between seek and playback state change
    void gate_poll_timer();

private:
    std::shared_ptr<audio_controller> playing; // controller

    PlayQueue &queue;
    std::shared_ptr<LyricsProjector> lp;

    QTimer *m_position_poll_timer = new QTimer(this); // connect() requires this to be a pointer

    std::atomic_bool m_slider_pressed {false};
};