#pragma once

#include "mediatypes.hpp"
#include "virtual/playerpresenter.hpp"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration>

#include <memory>
#include "objectproxybase.hpp"

// QObject MUST stay the first base: the QML prototype is resolved from it
class PlayerPresenterProxy : public QObject,
                             public ObjectProxyBase<PlayerPresenterProxy, PlayerPresenter>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PlayerPresenter)
    QML_SINGLETON

public:
    // mirror PlayerPresenterLI::PlaybackState; this needs to be undone so TODO
    enum class PlaybackState {
        paused,
        playing,
        stopped
    };
    Q_ENUM(PlaybackState)

    Q_PROPERTY(Types::Song currentTrack READ current_track NOTIFY trackChanged)

    Q_PROPERTY(QString title       READ title       NOTIFY trackChanged FINAL)
    Q_PROPERTY(QString artist      READ artist      NOTIFY trackChanged FINAL)
    Q_PROPERTY(QString album       READ album       NOTIFY trackChanged FINAL)
    Q_PROPERTY(QUrl    cover       READ cover       NOTIFY trackChanged)
    Q_PROPERTY(quint64 duration_ms READ duration_ms NOTIFY trackChanged)

    Q_PROPERTY(quint8  volume      READ volume      WRITE  set_volume      NOTIFY volumeChanged)

    Q_PROPERTY(bool isMediaLoaded  READ is_media_loaded NOTIFY mediaLoadedChanged)

    Q_PROPERTY(PlaybackState playbackState READ playback_state NOTIFY playbackStateChanged)

    Q_PROPERTY(double coverNuclearLightness  READ cover_nuclear_lightness NOTIFY lightnessLevelsChanged)
    Q_PROPERTY(double coverCentricLightness  READ cover_centric_lightness NOTIFY lightnessLevelsChanged)
    Q_PROPERTY(double coverMidringLightness  READ cover_midring_lightness NOTIFY lightnessLevelsChanged)
    Q_PROPERTY(double coverBordersLightness  READ cover_borders_lightness NOTIFY lightnessLevelsChanged)

    Q_PROPERTY(bool lightnessAwareness  READ lightness_awareness WRITE set_lightness_awareness_enabled NOTIFY lightnessAwarenessChanged)

    Q_PROPERTY(double coverPonderedLightness READ cover_pondered_lightness     NOTIFY lightnessLevelsChanged)

    explicit PlayerPresenterProxy(QObject *parent = nullptr) : QObject(parent) {
        forward(SIGNAL(track_changed()),             SIGNAL(trackChanged()));
        forward(SIGNAL(lightness_levels_changed()),  SIGNAL(lightnessLevelsChanged()));
        forward(SIGNAL(volume_changed()),            SIGNAL(volumeChanged()));
        forward(SIGNAL(playback_state_changed()),    SIGNAL(playbackStateChanged()));
        forward(SIGNAL(media_loaded_changed()),      SIGNAL(mediaLoadedChanged()));
        forward(SIGNAL(seek_finished()),             SIGNAL(seekFinished()));
        forward(SIGNAL(lightness_awareness_changed()), SIGNAL(lightnessAwarenessChanged()));
    }

    Types::Song current_track() const { return call(&PlayerPresenter::current_track); }

    // todo: use covers::lightness::default_lightness_value as the fallback
    double cover_nuclear_lightness() const { return call_or(0.5, &PlayerPresenter::cover_nuclear_lightness); }
    double cover_centric_lightness() const { return call_or(0.5, &PlayerPresenter::cover_centric_lightness); }
    double cover_midring_lightness() const { return call_or(0.5, &PlayerPresenter::cover_midring_lightness); }
    double cover_borders_lightness() const { return call_or(0.5, &PlayerPresenter::cover_borders_lightness); }

    double cover_pondered_lightness() const { return call(&PlayerPresenter::cover_pondered_lightness); }

    // enabled by default
    bool lightness_awareness() const { return call_or(true, &PlayerPresenter::lightness_awareness); }
    void set_lightness_awareness_enabled(bool enabled) {
        call(&PlayerPresenter::set_lightness_awareness_enabled, enabled);
    }

    // a default Types::Song yields empty fields when no presenter is injected
    QString title()       const { return call(&PlayerPresenter::current_track).title; }
    QString artist()      const { return call(&PlayerPresenter::current_track).artist; }
    QString album()       const { return call(&PlayerPresenter::current_track).album; }
    QUrl    cover()       const { return call(&PlayerPresenter::current_track).coveruri(); }
    quint64 duration_ms() const { return call(&PlayerPresenter::current_track).duration; }

    quint8 volume()          const { return call(&PlayerPresenter::volume); }
    bool   is_media_loaded() const { return call(&PlayerPresenter::is_media_loaded); }

    PlaybackState playback_state() const {
        using pp = PlayerPresenter::PlaybackState;
        switch (call_or(pp::stopped, &PlayerPresenter::playback_state)) {
            case pp::paused:  return PlaybackState::paused;
            case pp::playing: return PlaybackState::playing;
            case pp::stopped: return PlaybackState::stopped;
        }
        Q_UNREACHABLE();
    }

    void set_volume(quint8 volume) { call(&PlayerPresenter::set_volume, volume); }

    Q_INVOKABLE void    toggleMute()                 { call(&PlayerPresenter::toggle_mute); }
    Q_INVOKABLE quint64 position_ms() const          { return call(&PlayerPresenter::position_ms); }
    Q_INVOKABLE void    seekTo(quint64 position_ms)  { call(&PlayerPresenter::seek_to, position_ms); }

    Q_INVOKABLE double backdropLightnessForRect(QRect rect) const {
        return call_or(0.5, &PlayerPresenter::backdrop_lightness_for_rect, rect);
    }


signals:
    void trackChanged();
    void positionChanged();
    void volumeChanged();
    void playbackStateChanged();
    void mediaLoadedChanged();
    void lightnessLevelsChanged();
    void seekFinished();
    void lightnessAwarenessChanged();

public slots:
    void raiseVolume() { call(&PlayerPresenter::raise_lower_volume,  5); }
    void lowerVolume() { call(&PlayerPresenter::raise_lower_volume, -5); }

    void play()  const { call(&PlayerPresenter::play); }
    void pause() const { call(&PlayerPresenter::pause); }
    void stop()  const { call(&PlayerPresenter::stop); }
    void next()  const { call(&PlayerPresenter::next); }
    void prev()  const { call(&PlayerPresenter::prev); }

    void togglePlayPause() const { call(&PlayerPresenter::toggle_play_pause); }

    // for 5s back/forward shortcuts
    void shortSeekBack()    const { call(&PlayerPresenter::relative_seek, -5000); }
    void shortSeekForward() const { call(&PlayerPresenter::relative_seek, +5000); }

    void saveVolume() const { call(&PlayerPresenter::save_volume); }
};
