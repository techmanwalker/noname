#pragma once

#include "mediatypes.hpp"
#include "virtual/playerpresenter.hpp"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration>

#include <memory>

class PlayerPresenterProxy : public QObject
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

    explicit PlayerPresenterProxy(QObject *parent = nullptr)
        : QObject(parent),
          m_presenter(s_injectedPresenter) // Copies shared_ptr, incrementing ref count
    {
        // an interface cannot feature the implementation signals, need to be referred to by name
        if (auto *concrete = dynamic_cast<QObject*>(m_presenter.get())) {
            connect(concrete, SIGNAL(track_changed()),         this, SIGNAL(trackChanged()));
            connect(concrete, SIGNAL(lightness_levels_changed()),         this, SIGNAL(lightnessLevelsChanged()));
            connect(concrete, SIGNAL(volume_changed()),        this, SIGNAL(volumeChanged()));
            connect(concrete, SIGNAL(playback_state_changed()), this, SIGNAL(playbackStateChanged()));
            connect(concrete, SIGNAL(media_loaded_changed()),   this, SIGNAL(mediaLoadedChanged()));
            connect(concrete, SIGNAL(seek_finished()), this, SIGNAL(seekFinished()));
            connect(concrete, SIGNAL(lightness_awareness_changed()), this, SIGNAL(lightnessAwarenessChanged()));
        }
    }

    static void inject(const std::shared_ptr<PlayerPresenter> &presenter) {
        s_injectedPresenter = presenter; // Ref count incremented, caller's instance unaffected
    }

    Types::Song current_track () const { 
        if (!m_presenter) return Types::Song {};
        
        return m_presenter->current_track();
    }

    // todo: use covers::lightness::default_lightness_value as the third ternary component
    double cover_nuclear_lightness() const { return m_presenter ?  m_presenter->cover_nuclear_lightness() : 0.5;       }
    double cover_centric_lightness() const { return m_presenter ?  m_presenter->cover_centric_lightness() : 0.5;       }
    double cover_midring_lightness() const { return m_presenter ?  m_presenter->cover_midring_lightness() : 0.5;       }
    double cover_borders_lightness() const { return m_presenter ?  m_presenter->cover_borders_lightness() : 0.5;       }

    double cover_pondered_lightness() const     { return m_presenter ? m_presenter->cover_pondered_lightness()  : 0.0; }

    // enabled by default
    bool lightness_awareness () const { return m_presenter ? m_presenter->lightness_awareness() : true; }
    void set_lightness_awareness_enabled (bool enabled) { if (m_presenter) m_presenter->set_lightness_awareness_enabled(enabled); }

    QString title()            const { return m_presenter ?  m_presenter->current_track().title     : QString(); }
    QString artist()           const { return m_presenter ?  m_presenter->current_track().artist    : QString(); }
    QString album()            const { return m_presenter ?  m_presenter->current_track().album     : QString(); }
    QUrl    cover()            const { if (!m_presenter) return {}; return m_presenter->current_track().coveruri();  }
    quint64 duration_ms()      const { return m_presenter ?  m_presenter->current_track().duration  : 0;         }

    quint8  volume()           const { return m_presenter ?  m_presenter->volume()                    : 0;   }
    bool    is_media_loaded()  const { return m_presenter && m_presenter->is_media_loaded();                 }

    PlaybackState playback_state() const {
        if (!m_presenter) return PlaybackState::stopped;

        using pp = PlayerPresenter::PlaybackState;
        switch (m_presenter->playback_state()) {
            case pp::paused:  return PlaybackState::paused;
            case pp::playing: return PlaybackState::playing;
            case pp::stopped: return PlaybackState::stopped;
        }
        Q_UNREACHABLE();
    }

    void set_volume(quint8 volume) { if (m_presenter) m_presenter->set_volume(volume); }
    
    Q_INVOKABLE void toggleMute () { if (m_presenter) m_presenter->toggle_mute();      }

    Q_INVOKABLE quint64 position_ms()          const { return m_presenter ? m_presenter->position_ms() : 0;  }
    Q_INVOKABLE void    seekTo (quint64 position_ms) { if (m_presenter) m_presenter->seek_to(position_ms);   }

    Q_INVOKABLE double backdropLightnessForRect (QRect rect) const {
        if (m_presenter) {
            return m_presenter->backdrop_lightness_for_rect(rect);
        } else {
            return 0.5; 
        }
    }

    Q_INVOKABLE double ringLightnessForRect (QRect rect) const {
        if (m_presenter) {
            return m_presenter->ring_lightness_for_rect(rect);
        } else {
            return 0.5;
        }
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
    void raiseVolume()  { if (m_presenter) m_presenter->raise_lower_volume( 5); }
    void lowerVolume()  { if (m_presenter) m_presenter->raise_lower_volume(-5); }

    void play()  const { if (m_presenter) m_presenter->play();  }
    void pause() const { if (m_presenter) m_presenter->pause(); }
    void stop()  const { if (m_presenter) m_presenter->stop();  }
    void next()  const { if (m_presenter) m_presenter->next();  }
    void prev()  const { if (m_presenter) m_presenter->prev();  }

    void togglePlayPause() const { if (m_presenter) m_presenter->toggle_play_pause(); }

    // for 5s back/forward shortcuts
    void shortSeekBack()    const { if (m_presenter) m_presenter->relative_seek(- 5000); }
    void shortSeekForward() const { if (m_presenter) m_presenter->relative_seek(+ 5000); }

    void saveVolume() const { if (m_presenter) m_presenter->save_volume(); }

private:
    std::shared_ptr<PlayerPresenter> m_presenter;
    inline static std::shared_ptr<PlayerPresenter> s_injectedPresenter = nullptr;
};
