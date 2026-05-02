#pragma once

#include <QObject>
#include <QUrl>
#include <QString>

class PlayerState : public QObject
{
    Q_OBJECT

    // Q_PROPERTY defines the magic variables that QML can read and listen
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY albumChanged)
    Q_PROPERTY(QUrl cover READ cover WRITE setCover NOTIFY coverChanged)
    Q_PROPERTY(qint64 duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)

public:
    explicit PlayerState(QObject *parent = nullptr);

    // Getters
    QString title() const;
    QString artist() const;
    QString album() const;
    QUrl cover() const;
    qint64 duration() const;
    qint64 position() const;

    // Setters (normally called from C++ logic when time or song changes)
    void setTitle(const QString &title);
    void setArtist(const QString &artist);
    void setAlbum(const QString &album);
    void setCover(const QUrl &cover);
    void setDuration(qint64 duration);
    void setPosition(qint64 position);

signals:
    // Needed signals for QML to be reactive
    void titleChanged();
    void artistChanged();
    void albumChanged();
    void coverChanged();
    void durationChanged();
    void positionChanged();

private:
    QString m_title;
    QString m_artist;
    QString m_album;
    QUrl m_cover;
    qint64 m_duration;
    qint64 m_position; // Current timestamp in ms
};