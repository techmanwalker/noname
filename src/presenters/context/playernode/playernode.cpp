
#include "playernode.hpp"

#include "audioengine-in.hpp"

#include "configuration.hpp"
#include "lyricsprojector.hpp"
#include "mediatypes.hpp"
#include "playqueue.hpp"

#include <QQmlEngine>
#include <atomic>

// Private constructor
PlayerNode::PlayerNode(
    QObject *parent, 
    std::shared_ptr<audio_engine> controller,
    std::shared_ptr<PlayQueue> pqueue ,
    std::shared_ptr<LyricsProjector> lyricsproj
)
    : QObject(parent),
      playing(controller),
      queue(pqueue),
      lp(lyricsproj)
{
    m_position_poll_timer->setInterval(10);

    connect(m_position_poll_timer, &QTimer::timeout, 
            this, &PlayerNode::positionChanged);
        
    connect(this, &PlayerNode::sliderPressedChanged,
            this, &PlayerNode::handleSliderPressedChanged);

    // load volume from conf file
    auto &conf = configuration::manager::instance();
    const auto lines = conf.read_lines(configuration::conf_file_type::volume);

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
QString PlayerNode::title()         const { return playing->current_track().title;          }
QString PlayerNode::artist()        const { return playing->current_track().artist;         }
QString PlayerNode::album()         const { return playing->current_track().album;          }
QUrl    PlayerNode::cover()         const { return playing->current_track().cover.uri();    }
quint64 PlayerNode::duration_ms()   const { return playing->current_track().duration;       }
quint64 PlayerNode::position_ms()   const { return playing->current_position_ms();          }
quint8  PlayerNode::volume()        const { return playing->current_volume();               }
bool    PlayerNode::isMediaLoaded() const { return playing->is_a_song_loaded();             }

PlayerNode::PlaybackState
PlayerNode::playbackState() const
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
PlayerNode::setPosition_ms(quint64 position)
{
    // If we touch this code path, it means that the user has moved the playhead manually.
    playing->set_position(position);
}

void
PlayerNode::setVolume(quint8 volume)
{
    playing->set_volume(volume);
}

void
PlayerNode::saveVolume () const
{
    
    configuration::manager::instance().write_lines(
        configuration::conf_file_type::volume,
        { QString::number(volume()) }
    );

}

// --- Playback controls ---

// Proxies for QML to be able to perform play, pause and more actions
void
PlayerNode::play() const {
    if (!queue->playhead().isValid() && queue->itemCount() > 0) {
        // if the playhead does not point to anything, play what's next (which is the beginning)
        queue->next();
    }

    playing->play();
}

void
PlayerNode::pause() const
{
    playing->pause();
}

void
PlayerNode::stop() const
{
    playing->stop();
}

void
PlayerNode::next() const
{
    queue->next();
}

void
PlayerNode::prev() const
{
    if (playing->current_position_ms() > 5000) {
        playing->set_position(0); // start over
        return;
    }

    queue->prev();
}

void
PlayerNode::load(Types::Song &song) const
{
    playing->load(song);
}

void
PlayerNode::notify_slider_pressed_change (bool pressed)
{
    m_slider_pressed.store(pressed);

    emit sliderPressedChanged ();
}

void
PlayerNode::gate_poll_timer ()
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
PlayerNode::handleTrackChanged()
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
    lp->repopulate_with_lyrics_for_file(playing->current_track().source.toLocalFile()); /*
        .then([this] (std::vector<std::string> lrc_lines) {

                // uncomment this to print the lyrics model contents
                debug::print(l_playerpresenter(), "Lyrics in the LyricManifest:");
                debug::print(l_playerpresenter(), debug::serialize(lrc_lines));
                
            });*/
}

void
PlayerNode::handlePlaybackStateChanged()
{
    emit playbackStateChanged();

    gate_poll_timer();
}

void
PlayerNode::handleSliderPressedChanged()
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