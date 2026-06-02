#include "playbackpresentation.hpp"
#include <QQmlEngine>

// meyers singleton
PlaybackPresentation &PlaybackPresentation::instance() {
    static PlaybackPresentation s_instance;
    return s_instance;
}

// qml factory
PlaybackPresentation *PlaybackPresentation::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);

    PlaybackPresentation *inst = &instance();
    
    // transfer ownership to c++
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
    
    return inst;
}

// private constructor
PlaybackPresentation::PlaybackPresentation(QObject *parent)
    : QObject(parent),
      m_duration(0),
      m_position(0),
      m_volume(50)
{
}

QString PlaybackPresentation::title()       const { return m_title;    }
QString PlaybackPresentation::artist()      const { return m_artist;   }
QString PlaybackPresentation::album()       const { return m_album;    }
QUrl    PlaybackPresentation::cover()       const { return m_cover;    }
quint64 PlaybackPresentation::duration_ms() const { return m_duration; }
quint64 PlaybackPresentation::position_ms() const { return m_position; }
quint8  PlaybackPresentation::volume()      const { if (m_volume > 100) return 100; else return m_volume; }

void
PlaybackPresentation::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged();
    }
}

void
PlaybackPresentation::setArtist(const QString &artist)
{
    if (m_artist != artist) {
        m_artist = artist;
        emit artistChanged();
    }
}

void
PlaybackPresentation::setAlbum(const QString &album)
{
    if (m_album != album) {
        m_album = album;
        emit albumChanged();
    }
}

void
PlaybackPresentation::setCover(const QUrl &cover)
{
    if (m_cover != cover) {
        m_cover = cover;
        emit coverChanged();
    }
}

void
PlaybackPresentation::setDuration_ms(quint64 duration)
{
    if (m_duration != duration) {
        m_duration = duration;
        emit durationChanged();
    }
}

void
PlaybackPresentation::setPosition_ms(quint64 position)
{
    if (m_position != position) {
        m_position = position;
        emit positionChanged();
    }
}

void
PlaybackPresentation::setVolume(quint8 volume)
{
    if (m_volume != volume) {
        m_volume = ((volume > 100) ? 100 : volume);
        emit volumeChanged();
    }
}