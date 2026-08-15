
#include "playerpresenter.hpp"

#include "audioengine.hpp"
#include "configuration.hpp"
#include "lyricsmanifest.hpp"
#include "mediatypes.hpp"
#include "playqueue.hpp"

#include <QQmlEngine>
#include <atomic>

Q_LOGGING_CATEGORY (l_playerpresenter, "noname.context.playerpresenter");

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
    : QObject(parent),
      playing(audio_engine::instance()),
      queue(PlayQueue::instance()),
      lm(LyricsManifest::instance())
{
    m_position_poll_timer->setInterval(10);

    connect(m_position_poll_timer, &QTimer::timeout, 
            this, &PlayerPresenter::positionChanged);

    // Listen to the audio controller; when metadata updates, notify the QML engine
    connect(&playing, &audio_engine::track_changed,
            this, &PlayerPresenter::handleTrackChanged);

    connect(&playing, &audio_engine::playback_state_changed,
            this, &PlayerPresenter::handlePlaybackStateChanged);

    connect(&playing, &audio_engine::seek_finished,
            this, &PlayerPresenter::positionChanged);

    connect(&playing, &audio_engine::volume_changed,
            this, &PlayerPresenter::volumeChanged);
        
    connect(this, &PlayerPresenter::sliderPressedChanged,
            this, &PlayerPresenter::handleSliderPressedChanged);

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
QString PlayerPresenter::title()         const { return playing.current_track().title;          }
QString PlayerPresenter::artist()        const { return playing.current_track().artist;         }
QString PlayerPresenter::album()         const { return playing.current_track().album;          }
QUrl    PlayerPresenter::cover()         const { return playing.current_track().cover.uri();    }
quint64 PlayerPresenter::duration_ms()   const { return playing.current_track().duration;       }
quint64 PlayerPresenter::position_ms()   const { return playing.current_position_ms();          }
quint8  PlayerPresenter::volume()        const { return playing.current_volume();               }
bool    PlayerPresenter::isMediaLoaded() const { return playing.is_a_song_loaded();             }

PlayerPresenter::PlaybackState
PlayerPresenter::playbackState() const
{
    using ae = audio_engine::playback_state;

    switch (playing.get_playback_state()) {
        case ae::paused:  return PlaybackState::paused;
        case ae::playing: return PlaybackState::playing;
        case ae::stopped: return PlaybackState::stopped;
    }

    Q_UNREACHABLE(); // audio_engine::playback_state has no other members
}


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
PlayerPresenter::saveVolume () const
{
    
    configuration::manager::instance().write_lines(
        configuration::conf_file_type::volume,
        { QString::number(volume()) }
    );

}

// --- Playback controls ---

// Proxies for QML to be able to perform play, pause and more actions
void
PlayerPresenter::play() const {
    if (!queue.playhead().isValid() && queue.itemCount() > 0) {
        // if the playhead does not point to anything, play what's next (which is the beginning)
        queue.next();
    }

    playing.play();
}

void
PlayerPresenter::pause() const
{
    playing.pause();
}

void
PlayerPresenter::stop() const
{
    playing.stop();
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

void
PlayerPresenter::load(Types::Song &song) const
{
    playing.load(song);
}

void
PlayerPresenter::notify_slider_pressed_change (bool pressed)
{
    m_slider_pressed.store(pressed);

    emit sliderPressedChanged ();
}

void
PlayerPresenter::gate_poll_timer ()
{
    using playback_state = audio_engine::playback_state;
    switch(playing.get_playback_state()) {
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
PlayerPresenter::handleTrackChanged()
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
    lm.repopulate_with_lyrics_for_file(playing.current_track().source.toLocalFile())
        .then([this] () {
                std::vector<std::string> debug_lrc_lines;

                // serialize the logical lyrics back to .lrc only to print it back
                for (lyric single_timestamp_line : lm.current_lines()) {
                    debug_lrc_lines.emplace_back(
                        "[" + single_timestamp_line.ts.as_string() + "] " 
                        + single_timestamp_line.text.toStdString());
                };

                /*
                debug::print(l_playerpresenter(), "Lyrics in the LyricManifest:");
                debug::print(l_playerpresenter(), debug::serialize(debug_lrc_lines));*/
                
            });
}

void
PlayerPresenter::handlePlaybackStateChanged()
{
    emit playbackStateChanged();

    gate_poll_timer();
}

void
PlayerPresenter::handleSliderPressedChanged()
{
    // write only from qml so no need to emit signal here

    // avoid ui and audioengine fighting to print their position
    // this point of execution means that the pressed state changed
    // as the name suggests
    if (m_slider_pressed.load(std::memory_order_relaxed)) {
        m_position_poll_timer->stop();
        return;
    } else {
        connect (&playing, &audio_engine::seek_finished,
                this, [this] () {
                    gate_poll_timer(); // can freely decide
                });
    }

}