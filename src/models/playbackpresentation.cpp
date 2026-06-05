
#include "playbackpresentation.hpp"
#include "playbackcontroller.hpp"
#include <QQmlEngine>
#include <qjsengine.h>

// meyers singleton
PlaybackPresentation &PlaybackPresentation::instance() {
    static PlaybackPresentation s_instance;
    return s_instance;
}

// qml factory
PlaybackPresentation *PlaybackPresentation::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine);

    PlaybackPresentation *inst = &instance();

    if (qmlEngine) {
        qmlEngine->addImageProvider("covers", &playback_controller::instance().m_cover_provider);
    }
    
    // transfer ownership to c++; don't dare to destroy these
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
    QJSEngine::setObjectOwnership(&playback_controller::instance().m_cover_provider, QJSEngine::CppOwnership);

    
    return inst;
}

// Private constructor
PlaybackPresentation::PlaybackPresentation(QObject *parent)
    : QObject(parent)
{
    // Listen to the audio controller; when metadata updates, notify the QML engine
    connect(&playing, &playback_controller::track_changed,
            this, &PlaybackPresentation::handleTrackChanged);

    connect(&playing, &playback_controller::volume_changed,
            this, &PlaybackPresentation::handleVolumeChangedInController);

    connect(&playing, &playback_controller::playback_state_changed,
            this, &PlaybackPresentation::playbackStateChanged);

    connect(&playing, &playback_controller::position_poll_requested,
            this, &PlaybackPresentation::positionChanged);
}

QString PlaybackPresentation::title()       const { return playing.current_track().title;    }
QString PlaybackPresentation::artist()      const { return playing.current_track().artist;   }
QString PlaybackPresentation::album()       const { return playing.current_track().album;    }
QUrl    PlaybackPresentation::cover()       const { return playing.current_track().cover;    }
quint64 PlaybackPresentation::duration_ms() const { return playing.current_track().duration; }
quint64 PlaybackPresentation::position_ms() const { return playing.current_position_ms();    }
quint8  PlaybackPresentation::volume()      const { return playing.current_volume();         }
QMediaPlayer::PlaybackState
PlaybackPresentation::playbackState() const
{
    return playing.playback_state();
}

void
PlaybackPresentation::setPosition_ms(quint64 position)
{
    // If we touch this code path, it means that the user has moved the playhead manually.
    if (playing.current_position_ms() != position) {     
        // backend moves the playhead synchronously
        playing.set_position(position);
        
        // trigger the signal to notify the GUI that the local value has changed due to manual drag
        emit positionChanged();
    }
}

void
PlaybackPresentation::setVolume(quint8 volume)
{
    playing.set_volume(volume);
}

// --- Playback controls ---

// Proxies for QML to be able to perform play, pause and more actions
void
PlaybackPresentation::play() const {
    playing.play();
}

void
PlaybackPresentation::pause() const
{
    playing.pause();
}

// --- Signal handlers

void
PlaybackPresentation::handleTrackChanged()
{
    
    // Suddenly awake the QML declarative tree
    emit titleChanged();
    emit artistChanged();
    emit albumChanged();
    emit coverChanged();
    emit durationChanged();
    emit positionChanged();
}

void
PlaybackPresentation::handleVolumeChangedInController()
{
    emit volumeChanged();
}