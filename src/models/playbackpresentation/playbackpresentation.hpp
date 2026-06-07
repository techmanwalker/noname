#pragma once

#include <QObject>
#include <QUrl>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QMediaPlayer>
#include <atomic>

#include "playbackcontroller.hpp"

// forward declarations
class QQmlEngine;
class QJSEngine;

/**
    @class PlaybackPresentation
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
class PlaybackPresentation : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Q_PROPERTY defines the magic variables that QML can read and listen
    Q_PROPERTY(QString title       READ title       NOTIFY titleChanged)
    Q_PROPERTY(QString artist      READ artist      NOTIFY artistChanged)
    Q_PROPERTY(QString album       READ album       NOTIFY albumChanged)
    Q_PROPERTY(QUrl    cover       READ cover       NOTIFY coverChanged)
    Q_PROPERTY(quint64 duration_ms READ duration_ms NOTIFY durationChanged)
    Q_PROPERTY(quint64 position_ms READ position_ms WRITE setPosition_ms    NOTIFY positionChanged)
    Q_PROPERTY(quint8  volume      READ volume      WRITE setVolume         NOTIFY volumeChanged)

    // governed from the QML side
    Q_PROPERTY(bool duration_slider_pressed READ duration_slider_pressed WRITE setDurationSliderPressed)

    Q_PROPERTY(QMediaPlayer::PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)

public:
    // disable copy and reassignment
    PlaybackPresentation(const PlaybackPresentation&) = delete;
    PlaybackPresentation &operator=(const PlaybackPresentation&) = delete;
    
    // singleton instantiation
    static PlaybackPresentation &instance();
    static PlaybackPresentation *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Getters
    QString title() const;
    QString artist() const;
    QString album() const;
    QUrl   cover() const;
    quint64 duration_ms() const;
    quint64 position_ms() const;
    quint8 volume() const;
    bool duration_slider_pressed() const;
    QMediaPlayer::PlaybackState playbackState() const;

    // Setters (normally called from C++ logic when time or song changes)
    void setPosition_ms(quint64 position);
    void setVolume (quint8 volume);
    void setDurationSliderPressed (bool pressed);

    // Playback controls
    Q_INVOKABLE void play() const;
    Q_INVOKABLE void pause() const;
    

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

    // Reverse signals; NOTIFY but from QML to C++
    void r_durationSliderPressedChanged(bool pressed);

public slots:
    // Formally expose as a metadata block receiver
    void handleTrackChanged();
    void handleVolumeChangedInController();

private:
    // Private constructor
    explicit PlaybackPresentation(QObject *parent = nullptr);

    playback_controller &playing = playback_controller::instance();

    std::atomic_bool m_duration_slider_pressed = false; // is the duration slider pressed or dragged?
};

// --- Cheats so the QQmlEngine doesn't hard kill the player every time it's closed
class __cover_provider_PROXY : public QQuickImageProvider {
public:
    __cover_provider_PROXY(std::shared_ptr<cover_provider> realProvider);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
private:
    std::shared_ptr<cover_provider> m_real;
};