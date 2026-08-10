#pragma once

#include "audioengine.hpp"
#include "playqueue.hpp"

#include <QObject>

#include <QtQmlIntegration/qqmlintegration.h>

// forward declarations
class QQmlEngine;
class QJSEngine;

/**
    @class PlayerPresenter
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
class PlayerPresenter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // direct mirror of audio_engine::playback_state
    enum class PlaybackState {
        paused,
        playing,
        stopped
    };
    
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

    // disable copy and reassignment
    PlayerPresenter(const PlayerPresenter&) = delete;
    PlayerPresenter &operator=(const PlayerPresenter&) = delete;
    
    // singleton instantiation
    static PlayerPresenter &instance();
    static PlayerPresenter *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Getters
    QString title() const;
    QString artist() const;
    QString album() const;
    QUrl   cover() const;
    quint64 duration_ms() const;
    quint64 position_ms() const;
    quint8 volume() const;
    PlaybackState playbackState() const;
    bool isMediaLoaded() const;

    // Setters (normally called from C++ logic when time or song changes)
    void setPosition_ms(quint64 position);
    void setVolume (quint8 volume);

    // Playback controls
    Q_INVOKABLE void play()  const;
    Q_INVOKABLE void pause() const;
    Q_INVOKABLE void stop()  const;
    Q_INVOKABLE void next()  const;
    Q_INVOKABLE void prev()  const;
    
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

public slots:
    // Formally expose as a metadata block receiver
    void handleTrackChanged();
    void handleQueuedTracksFinished();
    void handlePlaybackStateChanged();
    void handleVolumeChangedInController();

private:
    // Private constructor
    explicit PlayerPresenter(QObject *parent = nullptr);

    audio_engine &playing = audio_engine::instance();
    PlayQueue &queue = PlayQueue::instance();

    QTimer *m_position_poll_timer = new QTimer(this); // connect() requires this to be a pointer
};