
#include "playerpresenter.hpp"

#include "audioengine-in.hpp"
#include "lyricsmanifest-in.hpp"

#include "manager-in.hpp"
#include "mediatypes.hpp"
#include "playqueue-in.hpp"

#include <QQmlEngine>
#include <atomic>
#include <memory>

// Private constructor
PlayerPresenterLI::PlayerPresenterLI(
    QObject *parent, 
    std::shared_ptr<configuration::manager> confmanager,
    std::shared_ptr<audio_engine> controller,
    std::shared_ptr<PlayQueue> pqueue ,
    std::shared_ptr<LyricsManifest> lyricsproj
)
    : QObject(parent),
      cm(confmanager),
      playing(controller),
      queue(pqueue),
      lm(lyricsproj)
{
    m_position_poll_timer->setInterval(10);

    connect(m_position_poll_timer, &QTimer::timeout, 
            this, &PlayerPresenterLI::positionChanged);
        
    connect(this, &PlayerPresenterLI::sliderPressedChanged,
            this, &PlayerPresenterLI::handleSliderPressedChanged);

    // load volume from conf file
    const auto lines = cm->read_lines(configuration::conf_file_type::volume);

    // only the volume value, nothing else
    if (lines.size() == 1) {
        bool vOK = false; // was volume successfully read as an int?

        const int volume = lines[0].toInt(&vOK);

        if (vOK) {
            // audioengine will clamp it or manage however it is to be done
            setVolume(volume);
        }
    }
}

// Getters block
QString PlayerPresenterLI::title()         const { return playing->current_track().title;          }
QString PlayerPresenterLI::artist()        const { return playing->current_track().artist;         }
QString PlayerPresenterLI::album()         const { return playing->current_track().album;          }
QUrl    PlayerPresenterLI::cover()         const { return playing->current_track().cover.uri();    }
quint64 PlayerPresenterLI::duration_ms()   const { return playing->current_track().duration;       }
quint64 PlayerPresenterLI::position_ms()   const { return playing->current_position_ms();          }
quint8  PlayerPresenterLI::volume()        const { return playing->current_volume();               }
bool    PlayerPresenterLI::isMediaLoaded() const { return playing->is_a_song_loaded();             }

PlayerPresenterLI::PlaybackState
PlayerPresenterLI::playbackState() const
{
    using ae = audio_engine::playback_state;

    switch (playing->get_playback_state()) {
        case ae::paused:  return PlaybackState::paused;
        case ae::playing: return PlaybackState::playing;
        case ae::stopped: return PlaybackState::stopped;
    }

    Q_UNREACHABLE(); // audio_engine::playback_state has no other members
}


void
PlayerPresenterLI::setPosition_ms(quint64 position)
{
    // If we touch this code path, it means that the user has moved the playhead manually.
    playing->set_position(position);
}

void
PlayerPresenterLI::setVolume(quint8 volume)
{
    playing->set_volume(volume);
}

void
PlayerPresenterLI::saveVolume () const
{
    
    cm->write_lines(
        configuration::conf_file_type::volume,
        { QString::number(volume()) }
    );

}

// --- Playback controls ---

// Proxies for QML to be able to perform play, pause and more actions
void
PlayerPresenterLI::play() const {
    if (!queue->playhead().isValid() && queue->itemCount() > 0) {
        // if the playhead does not point to anything, play what's next (which is the beginning)
        queue->next();
    }

    playing->play();
}

void
PlayerPresenterLI::pause() const
{
    playing->pause();
}

void
PlayerPresenterLI::stop() const
{
    playing->stop();
}

void
PlayerPresenterLI::next() const
{
    queue->next();
}

void
PlayerPresenterLI::prev() const
{
    if (playing->current_position_ms() > 5000) {
        playing->set_position(0); // start over
        return;
    }

    queue->prev();
}

void
PlayerPresenterLI::load(Types::Song &song) const
{
    playing->load(song);
}

void
PlayerPresenterLI::notify_slider_pressed_change (bool pressed)
{
    m_slider_pressed.store(pressed);

    emit sliderPressedChanged ();
}

void
PlayerPresenterLI::gate_poll_timer ()
{
    using playback_state = audio_engine::playback_state;
    switch(playing->get_playback_state()) {
        case playback_state::playing:
            m_position_poll_timer->start();
            break;
        case playback_state::paused:
        case playback_state::stopped:
            m_position_poll_timer->stop();
            break;
    }
}

// --- Signal handlers

void
PlayerPresenterLI::handleTrackChanged()
{
    // note: only to update the ui
    
    // Suddenly awake the QML declarative tree
    emit titleChanged();
    emit artistChanged();
    emit albumChanged();
    emit coverChanged();
    emit durationChanged();
    emit positionChanged();
    emit mediaLoadedChanged();

    // read lyrics from audio file and update
    lm->repopulate_with_lyrics_for_file(playing->current_track().source.toLocalFile()); /*
        .then([this] (std::vector<std::string> lrc_lines) {

                // uncomment this to print the lyrics model contents
                debug::print(l_playerpresenter(), "Lyrics in the LyricManifest:");
                debug::print(l_playerpresenter(), debug::serialize(lrc_lines));
                
            });*/
}

void
PlayerPresenterLI::handlePlaybackStateChanged()
{
    emit playbackStateChanged();

    gate_poll_timer();
}

void
PlayerPresenterLI::handleSliderPressedChanged()
{
    // write only from qml so no need to emit signal here

    // avoid ui and audioengine fighting to print their position
    // this point of execution means that the pressed state changed
    // as the name suggests
    if (m_slider_pressed.load(std::memory_order_relaxed)) {
        m_position_poll_timer->stop();
        return;
    }

}