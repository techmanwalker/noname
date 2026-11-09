
#include "playerpresenter.hpp"

#include "audioengine-in.hpp"

#include "coverlogging.hpp"
#include "coverlightness.hpp"
#include "coverstorage-in.hpp"
#include "manager-in.hpp"
#include "mediatypes.hpp"
#include "playqueue-in.hpp"

#include <QImage>
#include <QPointer>
#include <QtConcurrent/QtConcurrent>

#include <memory>
#include <qfuture.h>
#include <utility>

Q_LOGGING_CATEGORY(l_playerpresenter, "playerpresenter")

// Private constructor
PlayerPresenterLI::PlayerPresenterLI(
    QObject *parent, 
    std::shared_ptr<configuration::manager> confmanager,
    std::shared_ptr<audio_engine> controller,
    std::shared_ptr<PlayQueue> pqueue,
    std::shared_ptr<WindowGeometry> wgeometry,
    std::shared_ptr<cover_storage_in> coverprovider
)
    : QObject(parent),
      cm(std::move(confmanager)),
      playing(std::move(controller)),
      queue(std::move(pqueue)),
      wi(std::move(wgeometry)),
      cp(std::move(coverprovider))
{
    connect (this, &PlayerPresenterLI::volume_changed,
             this, &PlayerPresenterLI::handle_volume_changed);
    // load volume from conf file
    const auto lines = cm->read_lines(configuration::conf_file_type::volume);

    // only the volume value, nothing else
    if (lines.size() == 1) {
        bool vOK = false; // was volume successfully read as an int?

        const int volume = lines[0].toInt(&vOK);

        if (vOK) {
            // audioengine will clamp it or manage however it is to be done
            set_volume(volume);
            m_last_nonzero_volume = volume;
        }
    }
}

// Getters block
const Types::Song & PlayerPresenterLI::current_track() const { return m_current_track; }

quint64 PlayerPresenterLI::position_ms()     const { return playing->current_position_ms();  }
quint8  PlayerPresenterLI::volume()          const { return playing->current_volume();       }
bool    PlayerPresenterLI::is_media_loaded() const { return playing->is_a_song_loaded();     }

double
PlayerPresenterLI::lightness_of_ring (covers::lightness::lightness_types ring) const
{
    if (!m_lightness_awareness) return covers::lightness::default_lightness_value;

    return covers::lightness::ring_of(m_cover_lightness_rings, ring).value;
}

double PlayerPresenterLI::cover_nuclear_lightness() const { return lightness_of_ring(covers::lightness::lightness_types::nuclear); }
double PlayerPresenterLI::cover_centric_lightness() const { return lightness_of_ring(covers::lightness::lightness_types::centric); }
double PlayerPresenterLI::cover_midring_lightness() const { return lightness_of_ring(covers::lightness::lightness_types::midring); }
double PlayerPresenterLI::cover_borders_lightness() const { return lightness_of_ring(covers::lightness::lightness_types::borders); }

double PlayerPresenterLI::cover_pondered_lightness() const { return m_cover_pondered_lightness; }

bool PlayerPresenterLI::lightness_awareness() const { return m_lightness_awareness; }
void PlayerPresenterLI::set_lightness_awareness_enabled(bool enabled) { m_lightness_awareness = enabled; emit lightness_awareness_changed(); }

QFuture<void>
PlayerPresenterLI::recompute_lightness_levels(const Types::Song &backdrop_source)
{
    if (!m_lightness_awareness) return QtFuture::makeReadyVoidFuture();

    // Last call wins: kill the previous request (best effort) and bump the epoch
    // so that whatever it still delivers is discarded.
    m_lightness_values_future.cancel();
    const quint64 epoch = ++m_lightness_values_epoch;

    m_lightness_values_future = cp->resolve(backdrop_source.source, {256, 256});

    return m_lightness_values_future.then(this, [this, epoch](QImage thumbnail) {
        // A newer recompute_lightness_levels() was issued while this one was in flight.
        if (epoch != m_lightness_values_epoch) { return; }

        using namespace covers::lightness;
        using enum lightness_types;

        covers::lightness::cover_rings rings;
        ring_of(rings, nuclear) = percentile_lightness(thumbnail, 65, 0.00, 0.15);
        ring_of(rings, centric) = percentile_lightness(thumbnail, 60, 0.15, 0.60);
        ring_of(rings, midring) = percentile_lightness(thumbnail, 60, 0.60, 0.80);
        ring_of(rings, borders) = percentile_lightness(thumbnail, 70, 0.80, 1.00);

        double cover_pondered_lightness = covers::lightness::pondered_lightness(rings);

        /*
        constexpr double precision = 17;

        using covers::lightness::lightness_types;
        
        qCDebug(l_playerpresenter) << "cover nuclear lightness: " << QString::number(ring_of(rings, nuclear).value, 'f', precision);
        qCDebug(l_playerpresenter) << "cover centric lightness: " << QString::number(ring_of(rings, centric).value, 'f', precision);
        qCDebug(l_playerpresenter) << "cover midring lightness: " << QString::number(ring_of(rings, midring).value, 'f', precision);
        qCDebug(l_playerpresenter) << "cover borders lightness: " << QString::number(ring_of(rings, borders).value, 'f', precision);

        qCDebug(l_playerpresenter) << "pondered cover lightness: " << QString::number(cover_pondered_lightness, 'f', precision);
        */

        // Reccheck once more befor writing to memory
        if (epoch != m_lightness_values_epoch) { return; }

        m_cover_lightness_rings = rings;
        m_cover_lightness_table = covers::lightness::lightness_table_of_image(thumbnail, 55);

        m_cover_pondered_lightness = cover_pondered_lightness;

        emit lightness_levels_changed();
    });
}

double
PlayerPresenterLI::backdrop_lightness_for_rect (QRect rect) const
{
    return m_cover_lightness_table.backdrop_lightness_for_rect (rect, QSize { wi->width(), wi->height() });
}

double
PlayerPresenterLI::ring_lightness_for_rect (QRect rect) const
{
    using namespace covers::lightness;

    const std::optional<lightness_types> ring = dominant_ring_for_rect(
        m_cover_lightness_rings,
        m_cover_lightness_table.m_aspect_ratio,
        rect,
        QSize { wi->width(), wi->height() });

    // Same "no data" value backdrop_lightness_for_rect gives when nothing is visible.
    return ring ? ring_of(m_cover_lightness_rings, *ring).value : default_lightness_value;
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
    quint64 track_duration = m_current_track.duration;
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
    // volume() + step is computed in int (integer promotion): range -128..327, no overflow
    const int target_volume = volume() + step;

    // the audioengine already clamps the target percent but needs quint8
    set_volume(static_cast<quint8>(std::clamp(target_volume, 0, 100)));
}

void
PlayerPresenterLI:: toggle_mute() const
{
    set_volume(
        volume() > 0 ? 0 : m_last_nonzero_volume
    );
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
    if (!queue->playhead().isValid() && queue->item_count() >= 0) {
        // if the playhead does not point to anything, play what's next (which is the beginning)
        queue->switch_to(0);
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

// --- Signal handlers

void
PlayerPresenterLI::handle_track_changed()
{
    QUrl current_track_source = playing->current_track_source();

    // get the song struct from the play queue
    m_current_track = queue->query_metadata_for_source(current_track_source).value_or({});

    // if not matched, this is a race condition or someone forgot to update this
    Q_ASSERT_X (
        playing->current_track_source() == m_current_track.source,
        "PlayerPresenterLI::handle_track_changed",
        std::string(
            std::string("The player presenter current track metadata was not updated. ") +
            std::string("audio_engine reports the track playing is ") + 
            playing->current_track_source().toString().toStdString() +
            std::string(" while the presenter internal struct is ") +
            (!m_current_track.source.isEmpty() ? m_current_track.source.toString().toStdString() : "{empty}")

        ).data()
    );

    // Suddenly awake the QML declarative tree
    emit track_changed();
    emit media_loaded_changed();

    recompute_lightness_levels(m_current_track);
}

void
PlayerPresenterLI::handle_volume_changed()
{
    quint8 vol = volume();

    if (vol > 0) m_last_nonzero_volume = vol;
}
