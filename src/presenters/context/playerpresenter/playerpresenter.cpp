
#include "playerpresenter.hpp"

#include "audioengine-in.hpp"

#include "coverlogging.hpp"
#include "coverluminance.hpp"
#include "manager-in.hpp"
#include "mediatypes.hpp"
#include "playqueue-in.hpp"

#include "thumbnails.hpp"

#include <QImage>
#include <QPointer>
#include <QtConcurrent/QtConcurrent>

#include <atomic>
#include <memory>
#include <utility>

Q_LOGGING_CATEGORY(l_playerpresenter, "noname.playerpresenter")

// Private constructor
PlayerPresenterLI::PlayerPresenterLI(
    QObject *parent, 
    std::shared_ptr<configuration::manager> confmanager,
    std::shared_ptr<audio_engine> controller,
    std::shared_ptr<PlayQueue> pqueue,
    std::shared_ptr<WindowGeometry> wgeometry
)
    : QObject(parent),
      cm(confmanager),
      playing(controller),
      queue(pqueue),
      wi(wgeometry)
{
    m_position_poll_timer->setInterval(10);

    connect(m_position_poll_timer, &QTimer::timeout, 
            this, &PlayerPresenterLI::positionChanged);
        
    connect(this, &PlayerPresenterLI::sliderPressedChanged,
            this, &PlayerPresenterLI::handleSliderPressedChanged);

    connect(this, &PlayerPresenterLI::coverChanged,
            this, &PlayerPresenterLI::recompute_lumas);

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

double
PlayerPresenterLI::luma_of_ring (covers::luminance::luma_types ring) const
{
    return covers::luminance::ring_of(m_cover_luma_rings, ring).value;
}

double PlayerPresenterLI::coverNuclearLuma() const { return luma_of_ring(covers::luminance::luma_types::nuclear);  }
double PlayerPresenterLI::coverCentricLuma() const { return luma_of_ring(covers::luminance::luma_types::centric);; }
double PlayerPresenterLI::coverMidringLuma() const { return luma_of_ring(covers::luminance::luma_types::midring);; }
double PlayerPresenterLI::coverBordersLuma() const { return luma_of_ring(covers::luminance::luma_types::borders);; }

double
PlayerPresenterLI::coverPonderedLuma() const
{
    return m_cover_pondered_luma;
}

void
PlayerPresenterLI::recompute_lumas()
{
    const QUrl source = playing->current_track().source;

    const CoverRef ref(source, 256);
    const QImage thumbnail = covers::disk::fetch_thumbnail(ref);

    using namespace covers::luminance;
    using enum luma_types;

    covers::luminance::cover_rings rings;
    ring_of(rings, nuclear) = percentile_luminance(thumbnail, 65, 0.00, 0.15);
    ring_of(rings, centric) = percentile_luminance(thumbnail, 60, 0.15, 0.60);
    ring_of(rings, midring) = percentile_luminance(thumbnail, 60, 0.60, 0.80);
    ring_of(rings, borders) = percentile_luminance(thumbnail, 70, 0.80, 1.00);

    double cover_pondered_luma = covers::luminance::pondered_luma(rings);

    /*
    constexpr double precision = 17;

    using covers::luminance::luma_types;
    
    qCDebug(l_playerpresenter) << "cover nuclear luma: " << QString::number(ring_of(rings, nuclear).value, 'f', precision);
    qCDebug(l_playerpresenter) << "cover centric luma: " << QString::number(ring_of(rings, centric).value, 'f', precision);
    qCDebug(l_playerpresenter) << "cover midring luma: " << QString::number(ring_of(rings, midring).value, 'f', precision);
    qCDebug(l_playerpresenter) << "cover borders luma: " << QString::number(ring_of(rings, borders).value, 'f', precision);

    qCDebug(l_playerpresenter) << "pondered cover luma: " << QString::number(cover_pondered_luma, 'f', precision);
    */

    m_cover_luma_rings = std::move(rings);
    m_cover_luma_table = covers::luminance::luma_table_of_image(thumbnail, 55);

    m_cover_pondered_luma = cover_pondered_luma;

    emit lumasChanged();
}

double
PlayerPresenterLI::backdropLumaForRect (QRect rect) const
{
    return m_cover_luma_table.backdrop_luma_for_rect (rect, QSize { wi->width(), wi->height() });
}

double
PlayerPresenterLI::ringLumaForRect (QRect rect) const
{
    using namespace covers::luminance;

    const std::optional<luma_types> ring = dominant_ring_for_rect(
        m_cover_luma_rings,
        m_cover_luma_table.m_aspect_ratio,
        rect,
        QSize { wi->width(), wi->height() });

    // Same "no data" value backdropLumaForRect gives when nothing is visible.
    return ring ? ring_of(m_cover_luma_rings, *ring).value : 0.0;
}

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