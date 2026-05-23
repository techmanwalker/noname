#pragma once

#include <QObject>
#include <QUrl>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class PlayerState : public QObject
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
    static PlayerState &instance();
    static PlayerState *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // guarantee single instance
    PlayerState(const PlayerState&) = delete;
    PlayerState &operator=(const PlayerState&) = delete;

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
    explicit PlayerState(QObject *parent = nullptr);
    
    QString m_title;
    QString m_artist;
    QString m_album;
    QUrl m_cover;
    quint64 m_duration; // Song duration in ms
    quint64 m_position; // Current timestamp in ms
    quint8 m_volume = 100; // volume, from 0 to 100
};