
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

Q_LOGGING_CATEGORY(l_playerpresenter, "playerpresenter")

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

    // a track could be playing right now
    if (playback_state() == PlaybackState::playing) {
        m_position_poll_timer->start();
    }

    connect(m_position_poll_timer, &QTimer::timeout, 
            this, &PlayerPresenterLI::position_changed);
        
    connect(this, &PlayerPresenterLI::slider_pressed_changed,
            this, &PlayerPresenterLI::gate_poll_timer);

    connect(this, &PlayerPresenterLI::cover_changed,
            this, &PlayerPresenterLI::recompute_lumas);

    // load volume from conf file
    const auto lines = cm->read_lines(configuration::conf_file_type::volume);

    // only the volume value, nothing else
    if (lines.size() == 1) {
        bool vOK = false; // was volume successfully read as an int?

        const int volume = lines[0].toInt(&vOK);

        if (vOK) {
            // audioengine will clamp it or manage however it is to be done
            set_volume(volume);
        }
    }
}

// Getters block
QString PlayerPresenterLI::title()         const { return m_current_track_metadata.title;        }
QString PlayerPresenterLI::artist()        const { return m_current_track_metadata.artist;       }
QString PlayerPresenterLI::album()         const { return m_current_track_metadata.album;        }
QUrl    PlayerPresenterLI::cover()         const { return m_current_track_metadata.cover.uri();  }
quint64 PlayerPresenterLI::duration_ms()   const { return m_current_track_metadata.duration;     }

quint64 PlayerPresenterLI::position_ms()     const { return playing->current_position_ms();  }
quint8  PlayerPresenterLI::volume()          const { return playing->current_volume();       }
bool    PlayerPresenterLI::is_media_loaded() const { return playing->is_a_song_loaded();     }

double
PlayerPresenterLI::luma_of_ring (covers::luminance::luma_types ring) const
{
    return covers::luminance::ring_of(m_cover_luma_rings, ring).value;
}

double PlayerPresenterLI::cover_nuclear_luma() const { return luma_of_ring(covers::luminance::luma_types::nuclear); }
double PlayerPresenterLI::cover_centric_luma() const { return luma_of_ring(covers::luminance::luma_types::centric); }
double PlayerPresenterLI::cover_midring_luma() const { return luma_of_ring(covers::luminance::luma_types::midring); }
double PlayerPresenterLI::cover_borders_luma() const { return luma_of_ring(covers::luminance::luma_types::borders); }

double PlayerPresenterLI::cover_pondered_luma() const { return m_cover_pondered_luma; }

void
PlayerPresenterLI::recompute_lumas()
{
    const QUrl source = playing->current_track_source();

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

    emit lumas_changed();
}

double
PlayerPresenterLI::backdrop_luma_for_rect (QRect rect) const
{
    return m_cover_luma_table.backdrop_luma_for_rect (rect, QSize { wi->width(), wi->height() });
}

double
PlayerPresenterLI::ring_luma_for_rect (QRect rect) const
{
    using namespace covers::luminance;

    const std::optional<luma_types> ring = dominant_ring_for_rect(
        m_cover_luma_rings,
        m_cover_luma_table.m_aspect_ratio,
        rect,
        QSize { wi->width(), wi->height() });

    // Same "no data" value backdrop_luma_for_rect gives when nothing is visible.
    return ring ? ring_of(m_cover_luma_rings, *ring).value : 0.0;
}

PlayerPresenterLI::PlaybackState
PlayerPresenterLI::playback_state() const
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
PlayerPresenterLI::set_position_ms(quint64 position) const
{
    // If we touch this code path, it means that the user has moved the playhead manually.
    playing->set_position(position);
}

void
PlayerPresenterLI::relative_seek(qint64 ms) const
{
    // upper clamp only, the engine clamps for step resulting in negative position
    quint64 current_position = position_ms();
    quint64 track_duration = duration_ms();
    qint64 ms_abs = std::abs(ms);

    // back/forward stepping by ms would wrap around or misbehave
    bool over_max_limit  = ms > 0 && 
        (current_position + ms_abs > track_duration ||  // seeked over track
         current_position + ms_abs < current_position); // wrap around, ms > 0

    if (over_max_limit) {
        next();
        return;
    }

    playing->relative_seek(ms);
}

void
PlayerPresenterLI::set_volume(quint8 volume) const
{
    playing->set_volume(volume);
}

void
PlayerPresenterLI::raise_lower_volume(qint8 step) const
{
    if (step >  100) step =  100;
    if (step < -100) step = -100;

    // allows counting from to -100 to 200 without letting the step data type hold too much
    
    qint16 target_volume = static_cast<qint16>(volume()) + static_cast<qint16>(step);

    // the audioengine already clamps the target percent but needs qint8

    quint8 clamped_volume;

    if (target_volume > 100) {
        clamped_volume = 100;
    } else if (target_volume < 0) {
        clamped_volume = 0;
    } else [[likely]] {
        clamped_volume = static_cast<quint8>(target_volume);
    }

    set_volume(std::move(clamped_volume));
}

void
PlayerPresenterLI::save_volume () const
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
    if (!queue->playhead().isValid() && queue->item_count() > 0) {
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
PlayerPresenterLI::toggle_play_pause() const
{
    if (playback_state() == PlaybackState::playing) {
        pause();
    } else {
        play();
    }
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

    emit slider_pressed_changed ();
}

void
PlayerPresenterLI::gate_poll_timer ()
{
    using playback_state = audio_engine::playback_state;

    if (m_slider_pressed.load(std::memory_order_relaxed)) {
        m_position_poll_timer->stop();
        return;
    }

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
PlayerPresenterLI::handle_track_changed()
{
    // note: only to update the ui

    // if not matched, this is a race condition or someone forgot to update this
    Q_ASSERT_X (
        playing->current_track_source() == m_current_track_metadata.source,
        "PlayerPresenterLI::handle_track_changed",
        std::string(
            std::string("The player presenter current track metadata was not updated. ") +
            std::string("audio_engine reports the track playing is ") + 
            playing->current_track_source().toString().toStdString() +
            std::string(" while the presenter internal struct is ") +
            m_current_track_metadata.source.toString().toStdString()

        ).data()
    );
    
    // Suddenly awake the QML declarative tree
    emit title_changed();
    emit artist_changed();
    emit album_changed();
    emit cover_changed();
    emit duration_changed();
    emit position_changed();
    emit media_loaded_changed();
}

void
PlayerPresenterLI::handle_playback_state_changed()
{
    emit playback_state_changed();

    gate_poll_timer();
}

bool
PlayerPresenterLI::acknowledge_current_song(const Types::Song &metadata)
{
    if (metadata.source != playing->current_track_source()) { return false; }

    m_current_track_metadata = metadata;

    handle_track_changed();

    return true;
}