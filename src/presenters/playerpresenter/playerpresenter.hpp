#pragma once

#include "audioengine-in.hpp"

#include "coverluminance.hpp"
#include "manager-in.hpp"
#include "playerpresenter-in.hpp"
#include "windowgeometry-in.hpp"

#include <QLoggingCategory>
#include <QObject>
#include <QTimer>
#include <QString>
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
        std::shared_ptr<WindowGeometry> wgeometry
    );
    
    using PlayerPresenter::PlaybackState;
    Q_ENUM(PlaybackState)

    // Getters
    QString title() const override;
    QString artist() const override;
    QString album() const override;
    QUrl    cover() const override;
    quint64 duration_ms() const override;
    quint64 position_ms() const override;
    quint8 volume() const override;
    PlaybackState playback_state() const override;
    bool is_media_loaded() const override;

    double cover_nuclear_luma() const override;
    double cover_centric_luma() const override;
    double cover_midring_luma() const override;
    double cover_borders_luma() const override;

    double cover_pondered_luma() const override;

    double backdrop_luma_for_rect (QRect rect) const override;
    double ring_luma_for_rect     (QRect rect) const override;

    // Setters (normally called from C++ logic when time or song changes)
    void set_position_ms (quint64 position) override;
    void set_volume (quint8 volume) override;

    // Call forwardings
    void load(Types::Song &song) const override;

    // Playback controls
    void play()  const override;
    void pause() const override;
    void stop()  const override;
    void next()  const override;
    void prev()  const override;

    // to stop the poll timer while scrubbing
    void notify_slider_pressed_change (bool pressed) override;

    // save volume level to disk on demand
    void save_volume () const override;

    // update the current song struct on main
    bool acknowledge_current_song (const Types::Song &metadata);
    
signals:
    // Needed signals for QML to be reactive
    void title_changed();
    void artist_changed();
    void album_changed();
    void cover_changed();
    void duration_changed();
    void position_changed();
    void volume_changed();
    void playback_state_changed();
    void media_loaded_changed();
    void slider_pressed_changed();
    void lumas_changed();
    void seek_finished();

public slots:
    // Formally expose as a metadata block receiver
    void handle_track_changed();
    void handle_playback_state_changed();
    void gate_poll_timer();
    void recompute_lumas();

private:
    std::shared_ptr<configuration::manager> cm; // configuration
    std::shared_ptr<audio_engine> playing; // controller
    std::shared_ptr<PlayQueue> queue; // tape roll
    std::shared_ptr<WindowGeometry> wi; // information about the window

    QTimer *m_position_poll_timer = new QTimer(this); // connect() requires this to be a pointer

    std::atomic_bool m_slider_pressed {false};

    // do not access these directly from outside, these are guarded
    Types::Song m_current_track_metadata;
    
    covers::luminance::cover_rings m_cover_luma_rings;
    covers::luminance::table_luma  m_cover_luma_table;
    double m_cover_pondered_luma = 0.5;

    double luma_of_ring (covers::luminance::luma_types ring) const;
};