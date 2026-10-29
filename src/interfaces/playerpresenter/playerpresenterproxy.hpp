#pragma once

#include "playerpresenter-in.hpp"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration>

#include <memory>
#include <qtmetamacros.h>

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

    Q_PROPERTY(QString title       READ title       NOTIFY titleChanged  FINAL)
    Q_PROPERTY(QString artist      READ artist      NOTIFY artistChanged FINAL)
    Q_PROPERTY(QString album       READ album       NOTIFY albumChanged  FINAL)
    Q_PROPERTY(QUrl    cover       READ cover       NOTIFY coverChanged)
    Q_PROPERTY(quint64 duration_ms READ duration_ms NOTIFY durationChanged)

    Q_PROPERTY(quint8  volume      READ volume      WRITE  set_volume      NOTIFY volumeChanged)

    Q_PROPERTY(bool isMediaLoaded  READ is_media_loaded NOTIFY mediaLoadedChanged)

    Q_PROPERTY(PlaybackState playbackState READ playback_state NOTIFY playbackStateChanged)

    Q_PROPERTY(double coverNuclearLuma READ cover_nuclear_luma NOTIFY lumasChanged)
    Q_PROPERTY(double coverCentricLuma READ cover_centric_luma NOTIFY lumasChanged)
    Q_PROPERTY(double coverMidringLuma READ cover_midring_luma NOTIFY lumasChanged)
    Q_PROPERTY(double coverBordersLuma READ cover_borders_luma NOTIFY lumasChanged)

    Q_PROPERTY(double coverPonderedLuma     READ cover_pondered_luma     NOTIFY lumasChanged)

    explicit PlayerPresenterProxy(QObject *parent = nullptr)
        : QObject(parent),
          m_presenter(s_injectedPresenter) // Copies shared_ptr, incrementing ref count
    {
        // an interface cannot feature the implementation signals, need to be referred to by name
        if (auto *concrete = dynamic_cast<QObject*>(m_presenter.get())) {
            connect(concrete, SIGNAL(title_changed()),         this, SIGNAL(titleChanged()));
            connect(concrete, SIGNAL(artist_changed()),        this, SIGNAL(artistChanged()));
            connect(concrete, SIGNAL(album_changed()),         this, SIGNAL(albumChanged()));
            connect(concrete, SIGNAL(cover_changed()),         this, SIGNAL(coverChanged()));
            connect(concrete, SIGNAL(lumas_changed()),         this, SIGNAL(lumasChanged()));
            connect(concrete, SIGNAL(duration_changed()),      this, SIGNAL(durationChanged()));
            connect(concrete, SIGNAL(volume_changed()),        this, SIGNAL(volumeChanged()));
            connect(concrete, SIGNAL(playback_state_changed()), this, SIGNAL(playbackStateChanged()));
            connect(concrete, SIGNAL(media_loaded_changed()),   this, SIGNAL(mediaLoadedChanged()));
            connect(concrete, SIGNAL(seek_finished()), this, SIGNAL(seekFinished()));
        }
    }

    static void inject(const std::shared_ptr<PlayerPresenter> &presenter) {
        s_injectedPresenter = presenter; // Ref count incremented, caller's instance unaffected
    }

    QString title()            const { return m_presenter ?  m_presenter->title()            : QString(); }
    QString artist()           const { return m_presenter ?  m_presenter->artist()           : QString(); }
    QString album()            const { return m_presenter ?  m_presenter->album()            : QString(); }
    QUrl    cover()            const { return m_presenter ?  m_presenter->cover()            : QUrl();    }

    double cover_nuclear_luma() const { return m_presenter ?  m_presenter->cover_nuclear_luma() : 0.5;       }
    double cover_centric_luma() const { return m_presenter ?  m_presenter->cover_centric_luma() : 0.5;       }
    double cover_midring_luma() const { return m_presenter ?  m_presenter->cover_midring_luma() : 0.5;       }
    double cover_borders_luma() const { return m_presenter ?  m_presenter->cover_borders_luma() : 0.5;       }

    double cover_pondered_luma() const     { return m_presenter ? m_presenter->cover_pondered_luma()  : 0.0; }

    quint64 duration_ms()      const { return m_presenter ?  m_presenter->duration_ms()      : 0;         }
    quint8  volume()           const { return m_presenter ?  m_presenter->volume()           : 0;         }
    bool    is_media_loaded()  const { return m_presenter && m_presenter->is_media_loaded();              }

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

    void set_volume(quint8 volume)         { if (m_presenter) m_presenter->set_volume(volume); }

    Q_INVOKABLE quint64 position_ms()       const { return m_presenter ? m_presenter->position_ms() : 0;     }
    Q_INVOKABLE void    seekTo (quint64 position) { if (m_presenter) m_presenter->set_position_ms(position); }

    Q_INVOKABLE double backdropLumaForRect (QRect rect) const {
        if (m_presenter) {
            return m_presenter->backdrop_luma_for_rect(rect);
        } else {
            return 0.5; 
        }
    }

    Q_INVOKABLE double ringLumaForRect (QRect rect) const {
        if (m_presenter) {
            return m_presenter->ring_luma_for_rect(rect);
        } else {
            return 0.5;
        }
    }

signals:
    void titleChanged();
    void artistChanged();
    void albumChanged();
    void coverChanged();
    void durationChanged();
    void positionChanged();
    void volumeChanged();
    void playbackStateChanged();
    void mediaLoadedChanged();
    void lumasChanged();
    void seekFinished();

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