#pragma once

#include "interfaceid.hpp"
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtCore>

namespace Types {
    struct Song;
}

class PlayerPresenter
{

public:
    // direct mirror of audio_controller::playback_state
    enum class PlaybackState {
        paused,
        playing,
        stopped
    };

    virtual ~PlayerPresenter () = default;

    virtual QString title() const = 0;
    virtual QString artist() const = 0;
    virtual QString album() const = 0;
    virtual QUrl   cover() const = 0;
    virtual quint64 duration_ms() const = 0;
    virtual quint64 position_ms() const = 0;
    virtual quint8 volume() const = 0;
    virtual PlaybackState playback_state() const = 0;
    virtual bool is_media_loaded() const = 0;

    /*  Percentile OkLab lightness of the current cover, in [0,1]. Backs the
        background darkener's contrast target. Refer to PlayerPresenterLI
        to assess its computation heuristics. */
    virtual double cover_nuclear_luma() const = 0;
    virtual double cover_centric_luma() const = 0;
    virtual double cover_midring_luma() const = 0;
    virtual double cover_borders_luma() const = 0;

    virtual double cover_pondered_luma() const = 0;

    // get the luma of the pixels behind this rect from the cover
    virtual double backdrop_luma_for_rect (QRect rect) const = 0;

    // luma of the ring that spans the bigger part of the area of this rect
    virtual double ring_luma_for_rect (QRect rect) const = 0;

    virtual void set_position_ms(quint64 position) const = 0;

    // respect to current pos
    // + seeks forward, - seeks back
    virtual void relative_seek (qint64 ms) const = 0;

    virtual void set_volume (quint8 volume) const = 0; // from 0 to 100
    virtual void raise_lower_volume (qint8 step) const = 0; // relative to current volume

    virtual void play()  const = 0;
    virtual void pause() const = 0;
    virtual void stop()  const = 0;
    virtual void next()  const = 0;
    virtual void prev()  const = 0;

    virtual void toggle_play_pause () const = 0;

    virtual void load(Types::Song &song) const = 0;

    virtual void notify_slider_pressed_change (bool pressed) = 0;
    virtual void save_volume () const = 0;
};

inline constexpr const char* PlayerPresenter_iid = interface_iid<"PlayerPresenter">.buf;
Q_DECLARE_INTERFACE(PlayerPresenter, PlayerPresenter_iid)