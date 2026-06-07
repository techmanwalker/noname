
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
        // fetch the original shared_ptr
        auto cxx_shared = playback_controller::instance().m_cover_provider;

        /*  Explanation of the proxy mechanism:

            QQmlEngine will take ownership of this proxy, and upon program teardown,
            will invoke 'delete' on it.
            As m_real is a std::shared_ptr, when the proxy is destroyed it will
            decrement the reference counter safely without prematurely freeing the
            displaced memory block where the qml engine thinks the cover_provider is.
        */
        auto *proxy = new __cover_provider_PROXY(cxx_shared);

        qmlEngine->addImageProvider("covers", proxy);
    }
    
    // Transfer ownership to C++; don't you dare to destroy these either.
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);

    // no need to use the CppOwnership sentence for the m_cover_provider because it's a shared_ptr
    // and qml can't destroy it anyway
    
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

    connect(&playing, &playback_controller::position_changed,
            this, &PlaybackPresentation::positionChanged);

    // Send the reverse signal to the controller when the slider is pressed or not
    connect(this, &PlaybackPresentation::r_durationSliderPressedChanged,
            &playing, &playback_controller::r_duration_slider_pressed_changed);
}

// Getters block
QString PlaybackPresentation::title()       const { return playing.current_track().title;    }
QString PlaybackPresentation::artist()      const { return playing.current_track().artist;   }
QString PlaybackPresentation::album()       const { return playing.current_track().album;    }
QUrl    PlaybackPresentation::cover()       const { return playing.current_track().cover;    }
quint64 PlaybackPresentation::duration_ms() const { return playing.current_track().duration; }
quint64 PlaybackPresentation::position_ms() const { return playing.current_position_ms();    }
quint8  PlaybackPresentation::volume()      const { return playing.current_volume();         }

bool    PlaybackPresentation::duration_slider_pressed() const { return m_duration_slider_pressed.load(); }

QMediaPlayer::PlaybackState PlaybackPresentation::playbackState() const { return playing.playback_state(); }


void
PlaybackPresentation::setPosition_ms(quint64 position)
{
    // If we touch this code path, it means that the user has moved the playhead manually.
    playing.set_position(position);
}

void
PlaybackPresentation::setVolume(quint8 volume)
{
    playing.set_volume(volume);
}

void
PlaybackPresentation::setDurationSliderPressed(bool pressed)
{
    m_duration_slider_pressed.store(pressed);

    emit r_durationSliderPressedChanged(pressed);
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


// --- Cheats so the QQmlEngine doesn't hard kill the player every time it's closed
// Please don't pay too much attention to the syntax here. Qt threatened me to do this.
__cover_provider_PROXY::__cover_provider_PROXY(
    std::shared_ptr<cover_provider> realProvider
    )
    : QQuickImageProvider(QQuickImageProvider::Image),
      m_real(realProvider) 
{
}

// Simply redirect requests to the real cover provider.
QImage 
__cover_provider_PROXY::requestImage(
    const QString &id,
    QSize *size,
    const QSize &requestedSize)
{
        return m_real->requestImage(id, size, requestedSize);
}