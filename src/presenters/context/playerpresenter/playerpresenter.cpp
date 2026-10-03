
#include "playerpresenter.hpp"

#include "audioengine-in.hpp"

#include "coverluminance.hpp"
#include "manager-in.hpp"
#include "mediatypes.hpp"
#include "playqueue-in.hpp"

#include "covertransform.hpp"
#include "thumbnails.hpp"

#include <QImage>
#include <QPointer>
#include <QtConcurrent/QtConcurrent>

#include <atomic>
#include <memory>

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

double PlayerPresenterLI::coverNuclearLuma() const { return m_cover_lumas.nuclear_luma.value; }
double PlayerPresenterLI::coverCentricLuma() const { return m_cover_lumas.centric_luma.value; }
double PlayerPresenterLI::coverMidringLuma() const { return m_cover_lumas.midring_luma.value; }
double PlayerPresenterLI::coverBordersLuma() const { return m_cover_lumas.borders_luma.value; }

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

    covers::luminance::cover_luma lumas {
        covers::luminance::luma (
            covers::luminance::percentile_luminance(thumbnail, 65, 0.00, 0.15),
            0.00,
            0.15
        ),

        covers::luminance::luma (
            covers::luminance::percentile_luminance(thumbnail, 60, 0.15, 0.60),
            0.15,
            0.60
        ),

        covers::luminance::luma (
            covers::luminance::percentile_luminance(thumbnail, 60, 0.60, 0.80),
            0.60,
            0.80
        ),

        covers::luminance::luma (
            covers::luminance::percentile_luminance(thumbnail, 70, 0.80, 1.00),
            0.80,
            1.00
        )
    };

    double cover_pondered_luma = covers::luminance::pondered_luma(lumas);

    constexpr double precision = 17;

    qCDebug(l_playerpresenter) << "cover nuclear luma: " << QString::number(lumas.nuclear_luma.value, 'f', precision);
    qCDebug(l_playerpresenter) << "cover centric luma: " << QString::number(lumas.centric_luma.value, 'f', precision);
    qCDebug(l_playerpresenter) << "cover midring luma: " << QString::number(lumas.midring_luma.value, 'f', precision);
    qCDebug(l_playerpresenter) << "cover borders luma: " << QString::number(lumas.borders_luma.value, 'f', precision);

    qCDebug(l_playerpresenter) << "pondered cover luma: " << QString::number(cover_pondered_luma, 'f', precision);

    m_cover_lumas = std::move(lumas);
    m_cover_pondered_luma = cover_pondered_luma;

    emit lumasChanged();
}

double
PlayerPresenterLI::backingLumaForRect (QRect rect) const
{
    const QUrl source = playing->current_track().source;

    const CoverRef ref(source, 256);
    const QImage thumbnail = covers::disk::fetch_thumbnail(ref);

    if (thumbnail.isNull())
        return 0.0; // no cover: pick whatever the "no backing" answer should be

    // `rect` is in surface coordinates, like the surface itself.
    const QSizeF surface(wi->width(), wi->height());

    // The part of the thumbnail the Background actually shows: the largest
    // centered region with the surface's aspect ratio. Not cropped, just measured.
    const QSizeF fit = covers::transform::largest_aspect_size(thumbnail.size(),
                                           surface.width(), surface.height());
    if (fit.isEmpty())
        return 0.0; // zero-sized surface

    const double  scale = fit.width() / surface.width();
    const QPointF origin((thumbnail.width()  - fit.width())  / 2.0,
                         (thumbnail.height() - fit.height()) / 2.0);

    const QRectF mapped(origin.x() + rect.x() * scale,
                        origin.y() + rect.y() * scale,
                        rect.width()  * scale,
                        rect.height() * scale);

    // Outward to whole pixels so every touched pixel counts; the intersection
    // also handles rects that hang past the surface edge.
    const QRect region = mapped.toAlignedRect().intersected(thumbnail.rect());
    if (region.isEmpty())
        return 0.0;

    const QImage backing = thumbnail.copy(region); // only the small sub-region

    return covers::luminance::percentile_luminance(backing, 55);
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