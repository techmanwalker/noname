#pragma once

#include "audioengine-in.hpp"

#include "coverstorage-in.hpp"
#include "coverluminance.hpp"
#include "manager-in.hpp"
#include "playerpresenter-in.hpp"
#include "windowgeometry-in.hpp"

#include <QFuture>
#include <QImage>
#include <QLoggingCategory>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <atomic>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(l_playerpresenter)

class LyricsManifest;
class PlayQueue;

namespace Types {
    struct Song;
}

/**
    @class PlayerPresenterLI
    @brief Declarative and reactive representation of the playback state for the user interface.

    This class acts as the view model within the application architecture,
    serving as the single source of truth for visual components (QML) regarding the current 
    state of the audio engine. Its fundamental purpose is to structure and expose playback data 
    efficiently, completely decoupling business logic and audio processing from the interface.

    @section Data flow (detachment)
    The information flow is strictly compartmentalized on the C++ axis:
    - Input (Visual Update): A low-level audio controller (e.g., %PlaybackController) 
    manipulates the audio hardware/backend and updates the data of this class through its public slots 
    (@ref updateMetadata, @ref updatePosition).
    - Output (User Requests): When the user interacts with the interface (such as dragging a progress 
    bar or changing the volume), the class does not modify the internal state directly; instead, it emits 
    request signals (@ref requestSeek, @ref requestVolumeChange) so that the C++ controller validates and executes 
    the action on the audio engine.

    @note This class is designed as a Singleton managed by the QML engine (@c QML_SINGLETON) and exposes 
    read-only properties for static metadata, ensuring that the interface cannot corrupt 
    the current media state directly.
*/
class PlayerPresenterLI : public QObject, public PlayerPresenter
{
    Q_OBJECT
    Q_INTERFACES(PlayerPresenter)

public:
    explicit PlayerPresenterLI(
        QObject *parent,
        std::shared_ptr<configuration::manager> confmanager,
        std::shared_ptr<audio_engine> controller,
        std::shared_ptr<PlayQueue> pqueue,
        std::shared_ptr<WindowGeometry> wgeometry,
        std::shared_ptr<cover_storage_in> coverprovider
    );
    
    using PlayerPresenter::PlaybackState;
    Q_ENUM(PlaybackState)

    // Getters
    const Types::Song & current_track () const override;
    quint64 position_ms() const override;
    quint8 volume() const override;
    PlaybackState playback_state() const override;
    bool is_media_loaded() const override;

    double cover_nuclear_luma() const override;
    double cover_centric_luma() const override;
    double cover_midring_luma() const override;
    double cover_borders_luma() const override;
    
    double cover_pondered_luma() const override;

    bool luma_awareness () const override;
    void set_luma_awareness_enabled (bool enabled) override;

    double backdrop_luma_for_rect (QRect rect) const override;
    double ring_luma_for_rect     (QRect rect) const override;

    void set_position_ms (quint64 position) const override;
    void relative_seek (qint64 ms) const override;

    void set_volume (quint8 volume) const override;
    void raise_lower_volume (qint8 step) const override;
    void toggle_mute () const override;

    // Call forwardings
    void load(Types::Song &song) const override;

    // Playback controls
    void play()  const override;
    void pause() const override;
    void stop()  const override;
    void next()  const override;
    void prev()  const override;

    void toggle_play_pause () const override;

    // save volume level to disk on demand
    void save_volume () const override;
    
signals:
    // Needed signals for QML to be reactive
    void track_changed();
    void volume_changed();
    void playback_state_changed();
    void media_loaded_changed();
    void lumas_changed();
    void seek_finished();
    void lightness_awareness_changed();

public slots:
    // Formally expose as a metadata block receiver
    void handle_track_changed();

    void handle_volume_changed();

private slots:
    QFuture<void> recompute_lumas(const Types::Song &backdrop_source);

private:
    std::shared_ptr<configuration::manager> cm; // configuration
    std::shared_ptr<audio_engine> playing; // controller
    std::shared_ptr<PlayQueue> queue; // tape roll
    std::shared_ptr<WindowGeometry> wi; // information about the window
    std::shared_ptr<cover_storage_in> cp; // cover provider

    std::atomic_bool m_slider_pressed {false};

    // do not access these directly from outside, these are guarded
    Types::Song m_current_track;
    
    covers::luminance::cover_rings m_cover_luma_rings;
    covers::luminance::table_luma  m_cover_luma_table;
    double m_cover_pondered_luma = covers::luminance::default_lightness_value;

    // latest recompute_lumas() request; older ones are cancelled/ignored
    QFuture<QImage> m_lumas_future;
    quint64 m_lumas_epoch = 0;

    // if false, all luma recalculation is skipped and assumed to be uninitialized
    bool m_luma_awareness = false;

    quint8 m_last_nonzero_volume { 65 };

    double luma_of_ring (covers::luminance::luma_types ring) const;
};
