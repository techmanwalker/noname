
#include "playerpresenter.hpp"

#include <QQmlEngine>

// meyers singleton
PlayerPresenter &PlayerPresenter::instance() {
    static PlayerPresenter s_instance;
    return s_instance;
}

// qml factory
PlayerPresenter *PlayerPresenter::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine);

    PlayerPresenter *inst = &instance();
    
    // Transfer ownership to C++; don't you dare to destroy these either.
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);

    // no need to use the CppOwnership sentence for the m_cover_provider because it's a shared_ptr
    // and qml can't destroy it anyway
    
    return inst;
}

// Private constructor
PlayerPresenter::PlayerPresenter(QObject *parent)
    : QObject(parent)
{
    // Listen to the audio controller; when metadata updates, notify the QML engine
    connect(&playing, &audio_engine::track_changed,
            this, &PlayerPresenter::handleTrackChanged);

    connect(&playing, &audio_engine::volume_changed,
            this, &PlayerPresenter::handleVolumeChangedInController);

    connect(&playing, &audio_engine::playback_state_changed,
            this, &PlayerPresenter::playbackStateChanged);

    connect(&playing, &audio_engine::position_changed,
            this, &PlayerPresenter::positionChanged);

    // Send the reverse signal to the controller when the slider is pressed or not
    connect(this, &PlayerPresenter::r_durationSliderPressedChanged,
            &playing, &audio_engine::r_duration_slider_pressed_changed);
}

// Getters block
QString PlayerPresenter::title()         const { return playing.current_track().title;    }
QString PlayerPresenter::artist()        const { return playing.current_track().artist;   }
QString PlayerPresenter::album()         const { return playing.current_track().album;    }
QUrl    PlayerPresenter::cover()         const { return playing.current_track().cover;    }
quint64 PlayerPresenter::duration_ms()   const { return playing.current_track().duration; }
quint64 PlayerPresenter::position_ms()   const { return playing.current_position_ms();    }
quint8  PlayerPresenter::volume()        const { return playing.current_volume();         }
bool    PlayerPresenter::isMediaLoaded() const { return playing.is_a_song_loaded();       }

bool    PlayerPresenter::duration_slider_pressed() const { return m_duration_slider_pressed.load(); }

audio_engine::playback_state PlayerPresenter::playbackState() const { return playing.get_playback_state(); }


void
PlayerPresenter::setPosition_ms(quint64 position)
{
    // If we touch this code path, it means that the user has moved the playhead manually.
    playing.set_position(position);
}

void
PlayerPresenter::setVolume(quint8 volume)
{
    playing.set_volume(volume);
}

void
PlayerPresenter::setDurationSliderPressed(bool pressed)
{
    m_duration_slider_pressed.store(pressed);

    emit r_durationSliderPressedChanged(pressed);
}

// --- Playback controls ---

// Proxies for QML to be able to perform play, pause and more actions
void
PlayerPresenter::play() const {
    playing.play();
}

void
PlayerPresenter::pause() const
{
    playing.pause();
}

void
PlayerPresenter::next() const
{
    queue.next();
}

void
PlayerPresenter::prev() const
{
    if (playing.current_position_ms() > 5000) {
        playing.set_position(0); // start over
        return;
    }

    queue.prev();
}

// --- Signal handlers

void
PlayerPresenter::handleTrackChanged()
{
    
    // Suddenly awake the QML declarative tree
    emit titleChanged();
    emit artistChanged();
    emit albumChanged();
    emit coverChanged();
    emit durationChanged();
    emit positionChanged();
    emit mediaLoadedChanged();
}

void
PlayerPresenter::handleVolumeChangedInController()
{
    emit volumeChanged();
}