
#include "playerpresenter.hpp"

#include "virtual/audioengine.hpp"

#include "logging/l_coverprovider.hpp"
#include "presentation/lightness.hpp"
#include "virtual/coverstorage.hpp"
#include "virtual/manager.hpp"
#include "mediatypes.hpp"
#include "virtual/playqueue.hpp"

#include <QImage>
#include <QPointer>
#include <QtConcurrent/QtConcurrent>

#include <memory>
#include <utility>

Q_LOGGING_CATEGORY(l_playerpresenter, "playerpresenter")

// Private constructor
PlayerPresenterLI::PlayerPresenterLI(
    QObject *parent,
    std::shared_ptr<configuration::manager> confmanager,
    std::shared_ptr<audio_engine> audioengine,
    std::shared_ptr<PlayQueue> playqueue,
    std::shared_ptr<WindowGeometry> windowgeometry,
    std::shared_ptr<cover_storage> coverprovider
)
    : QObject(parent),
      cm(std::move(confmanager)),
      ae(std::move(audioengine)),
      pq(std::move(playqueue)),
      wi(std::move(windowgeometry)),
      cp(std::move(coverprovider))
{
    connect (this, &PlayerPresenterLI::volume_changed,
             this, &PlayerPresenterLI::handle_volume_changed);
    // load volume from conf file
    const auto lines = cm->fetch_setting(configuration::setting::volume).value_or({});

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
// Last snapshot published by handle_track_changed(); may lag the engine by design.
const Types::Track & PlayerPresenterLI::current_track() const { return m_current_track; }

quint64 PlayerPresenterLI::position_ms()     const { return ae->current_position_ms();  }
quint8  PlayerPresenterLI::volume()          const { return ae->current_volume();       }
bool    PlayerPresenterLI::is_media_loaded() const { return ae->is_a_track_loaded();     }

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

namespace {

// wrrapper
struct luma_computation_results {
    covers::lightness::cover_rings     rings {};
    covers::lightness::lightness_table table {};
    double                             pondered_lightness {};
};

}

QFuture<void>
PlayerPresenterLI::recompute_lightness_levels(const Types::Track &backdrop_source)
{
    if (!m_lightness_awareness) return QtFuture::makeReadyVoidFuture();

    // Last call wins: kill the previous request (best effort) and bump the epoch
    // so that whatever it still delivers is discarded.
    m_lightness_values_future.cancel();
    const quint64 epoch = ++m_lightness_values_epoch;

    m_lightness_values_future = cp->resolve(backdrop_source.source, {0, 256});

    return m_lightness_values_future.then([epoch](QImage thumbnail) {
        using namespace covers::lightness;
        using enum lightness_types;

        covers::lightness::cover_rings rings;
        ring_of(rings, nuclear) = percentile_lightness(thumbnail, 65, 0.00, 0.15);
        ring_of(rings, centric) = percentile_lightness(thumbnail, 60, 0.15, 0.60);
        ring_of(rings, midring) = percentile_lightness(thumbnail, 60, 0.60, 0.80);
        ring_of(rings, borders) = percentile_lightness(thumbnail, 70, 0.80, 1.00);

        /*
        constexpr double precision = 17;

        using covers::lightness::lightness_types;

        qCDebug(l_playerpresenter) << "cover nuclear lightness: " << QString::number(ring_of(rings, nuclear).value, 'f', precision);
        qCDebug(l_playerpresenter) << "cover centric lightness: " << QString::number(ring_of(rings, centric).value, 'f', precision);
        qCDebug(l_playerpresenter) << "cover midring lightness: " << QString::number(ring_of(rings, midring).value, 'f', precision);
        qCDebug(l_playerpresenter) << "cover borders lightness: " << QString::number(ring_of(rings, borders).value, 'f', precision);

        qCDebug(l_playerpresenter) << "pondered cover lightness: " << QString::number(cover_pondered_lightness, 'f', precision);
        */

        return luma_computation_results {
            .rings=rings,
            .table=covers::lightness::lightness_table_of_image(thumbnail),
            .pondered_lightness=covers::lightness::pondered_lightness(rings)
        };
    }).then(this, [this, epoch](luma_computation_results results) {
        if (epoch != m_lightness_values_epoch) { return; }

        m_cover_lightness_rings    = results.rings;
        m_cover_lightness_table    = results.table;
        m_cover_pondered_lightness = results.pondered_lightness;

        emit lightness_levels_changed();
    });
}

double
PlayerPresenterLI::backdrop_lightness_for_rect (QRect rect) const
{
    return m_cover_lightness_table.backdrop_lightness_for_rect (rect, QSize { wi->width(), wi->height() });
}

PlayerPresenterLI::PlaybackState
PlayerPresenterLI::playback_state() const
{
    using ps = audio_engine::playback_state;

    switch (ae->get_playback_state()) {
        case ps::paused:  return PlaybackState::paused;
        case ps::playing: return PlaybackState::playing;
        case ps::stopped: return PlaybackState::stopped;
    }

    Q_UNREACHABLE(); // audio_engine::playback_state has no other members
}


QFuture<void>
PlayerPresenterLI::seek_to(quint64 position_ms) const
{
    // If we touch this code path, it means that the user has moved the playhead manually.
    return ae->seek_to(position_ms);
}

QFuture<void>
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
        return next();
    }

    return ae->relative_seek(ms);
}

QFuture<void>
PlayerPresenterLI::set_volume(quint8 volume) const
{
    return ae->set_volume(volume);
}

QFuture<void>
PlayerPresenterLI::raise_lower_volume(qint8 step) const
{
    // volume() + step is computed in int (integer promotion): range -128..327, no overflow
    const int target_volume = volume() + step;

    // the audioengine already clamps the target percent but needs quint8
    return set_volume(static_cast<quint8>(std::clamp(target_volume, 0, 100)));
}

QFuture<void>
PlayerPresenterLI:: toggle_mute() const
{
    return set_volume(
        volume() > 0 ? 0 : m_last_nonzero_volume
    );
}

QFuture<bool>
PlayerPresenterLI::save_volume () const
{

    return cm->write_setting (
        configuration::setting::volume,
        { QString::number(volume()) }
    );

}

// --- Playback controls ---

// Proxies for QML to be able to perform play, pause and more actions
QFuture<void>
PlayerPresenterLI::play() const {
    if (!pq->playhead().isValid() && pq->item_count() > 0) {
        // if the playhead does not point to anything, play what's next (which is the beginning)
        pq->switch_to(0);
    }

    return ae->play();
}

QFuture<void>
PlayerPresenterLI::pause() const
{
    return ae->pause();
}

QFuture<void>
PlayerPresenterLI::stop() const
{
    return ae->stop();
}

QFuture<void>
PlayerPresenterLI::next() const
{
    return pq->next();
}

QFuture<void>
PlayerPresenterLI::prev() const
{
    if (
        ae->current_position_ms() > 5000 ||

        // the only track in queue
        (pq->playhead().row() == 0 && pq->item_count() == 1)
    ) {
        return ae->seek_to(0);
    }

    return pq->prev();
}

QFuture<void>
PlayerPresenterLI::toggle_play_pause() const
{
    if (playback_state() == PlaybackState::playing) {
        return pause();
    } else {
        return play();
    }
}

QFuture<void>
PlayerPresenterLI::load(Types::Track &song) const
{
    return ae->load(song);
}

// --- Signal handlers

void
PlayerPresenterLI::handle_track_changed()
{
    const QUrl current_track_source = ae->current_track_source();

    // the queue may not know this track yet (startup): keep the source, metadata empty
    m_current_track = pq->query_metadata_for_source(current_track_source).value_or({});
    m_current_track.source = current_track_source;

    // Suddenly awake the QML declarative tree
    emit track_changed();

    recompute_lightness_levels(m_current_track);
}

void
PlayerPresenterLI::handle_volume_changed()
{
    quint8 vol = volume();

    if (vol > 0) m_last_nonzero_volume = vol;
}
