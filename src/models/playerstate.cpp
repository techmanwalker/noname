#include "playerstate.hpp"
#include <QQmlEngine>

// meyers singleton
PlayerState &PlayerState::instance() {
    static PlayerState s_instance;
    return s_instance;
}

// qml factory
PlayerState *PlayerState::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);

    PlayerState *inst = &instance();
    
    // transfer ownership to c++
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
    
    return inst;
}

// private constructor
PlayerState::PlayerState(QObject *parent)
    : QObject(parent),
      m_duration(0),
      m_position(0),
      m_volume(50)
{
}

QString PlayerState::title()       const { return m_title;    }
QString PlayerState::artist()      const { return m_artist;   }
QString PlayerState::album()       const { return m_album;    }
QUrl    PlayerState::cover()       const { return m_cover;    }
quint64 PlayerState::duration_ms() const { return m_duration; }
quint64 PlayerState::position_ms() const { return m_position; }
quint8  PlayerState::volume()      const { if (m_volume > 100) return 100; else return m_volume; }

void
PlayerState::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged();
    }
}

void
PlayerState::setArtist(const QString &artist)
{
    if (m_artist != artist) {
        m_artist = artist;
        emit artistChanged();
    }
}

void
PlayerState::setAlbum(const QString &album)
{
    if (m_album != album) {
        m_album = album;
        emit albumChanged();
    }
}

void
PlayerState::setCover(const QUrl &cover)
{
    if (m_cover != cover) {
        m_cover = cover;
        emit coverChanged();
    }
}

void
PlayerState::setDuration_ms(quint64 duration)
{
    if (m_duration != duration) {
        m_duration = duration;
        emit durationChanged();
    }
}

void
PlayerState::setPosition_ms(quint64 position)
{
    if (m_position != position) {
        m_position = position;
        emit positionChanged();
    }
}

void
PlayerState::setVolume(quint8 volume)
{
    if (m_volume != volume) {
        m_volume = ((volume > 100) ? 100 : volume);
        emit volumeChanged();
    }
}