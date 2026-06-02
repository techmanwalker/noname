#pragma once

#include <QObject>
#include <QUrl>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

// forward declarations
class QQmlEngine;
class QJSEngine;

/**
    @class PlaybackViewModel
    @brief Declarative and reactive representation of the playback state for the user interface.

    This class acts as the view model (ViewModel) within the application architecture,
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
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY albumChanged)
    Q_PROPERTY(QUrl cover READ cover WRITE setCover NOTIFY coverChanged)
    Q_PROPERTY(quint64 duration_ms READ duration_ms WRITE setDuration_ms NOTIFY durationChanged)
    Q_PROPERTY(quint64 position_ms READ position_ms WRITE setPosition_ms NOTIFY positionChanged)
    Q_PROPERTY(quint8 volume READ volume WRITE setVolume NOTIFY volumeChanged)

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

    // Setters (normally called from C++ logic when time or song changes)
    void setTitle(const QString &title);
    void setArtist(const QString &artist);
    void setAlbum(const QString &album);
    void setCover(const QUrl &cover);
    void setDuration_ms(quint64 duration);
    void setPosition_ms(quint64 position);
    void setVolume (quint8 volume);

signals:
    // Needed signals for QML to be reactive
    void titleChanged();
    void artistChanged();
    void albumChanged();
    void coverChanged();
    void durationChanged();
    void positionChanged();
    void volumeChanged();

private:
    // private constructor
    explicit PlaybackPresentation(QObject *parent = nullptr);
    
    QString m_title;
    QString m_artist;
    QString m_album;
    QUrl m_cover;
    quint64 m_duration; // Song duration in ms
    quint64 m_position; // Current timestamp in ms
    quint8 m_volume = 100; // volume, from 0 to 100
};