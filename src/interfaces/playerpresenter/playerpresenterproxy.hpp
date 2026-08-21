#pragma once

#include "playerpresenter.hpp"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

#include <memory>

class PlayerPresenterProxy : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(PlayerPresenter)
    QML_SINGLETON

public:
    // Mirrors PlayerPresenter::PlaybackState — redeclared rather than reused,
    // same approach PlayerNode already takes for audio_controller::playback_state:
    // this proxy doesn't inherit PlayerPresenter (it only holds one), so there's
    // no base-class relationship for moc to walk to find the original enum.
    enum class PlaybackState {
        paused,
        playing,
        stopped
    };
    Q_ENUM(PlaybackState)

    Q_PROPERTY(QString title       READ title       NOTIFY titleChanged)
    Q_PROPERTY(QString artist      READ artist      NOTIFY artistChanged)
    Q_PROPERTY(QString album       READ album       NOTIFY albumChanged)
    Q_PROPERTY(QUrl    cover       READ cover       NOTIFY coverChanged)
    Q_PROPERTY(quint64 duration_ms READ duration_ms NOTIFY durationChanged)
    Q_PROPERTY(quint64 position_ms READ position_ms WRITE setPosition_ms NOTIFY positionChanged)
    Q_PROPERTY(quint8  volume      READ volume      WRITE setVolume      NOTIFY volumeChanged)
    Q_PROPERTY(bool isMediaLoaded  READ isMediaLoaded NOTIFY mediaLoadedChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)

    explicit PlayerPresenterProxy(QObject *parent = nullptr)
        : QObject(parent),
          m_presenter(s_injectedPresenter) // Copies shared_ptr, incrementing ref count
    {
        // Opposite direction from the LyricsProjector/LocalLibrary proxies:
        // going interface* -> QObject*, which qobject_cast cannot do (it only
        // casts FROM a QObject). This is a genuine cross-cast between sibling
        // bases of the same PlayerNode object, hence dynamic_cast here.
        if (auto *concrete = dynamic_cast<QObject*>(m_presenter.get())) {
            connect(concrete, SIGNAL(titleChanged()),         this, SIGNAL(titleChanged()));
            connect(concrete, SIGNAL(artistChanged()),        this, SIGNAL(artistChanged()));
            connect(concrete, SIGNAL(albumChanged()),         this, SIGNAL(albumChanged()));
            connect(concrete, SIGNAL(coverChanged()),         this, SIGNAL(coverChanged()));
            connect(concrete, SIGNAL(durationChanged()),      this, SIGNAL(durationChanged()));
            connect(concrete, SIGNAL(positionChanged()),      this, SIGNAL(positionChanged()));
            connect(concrete, SIGNAL(volumeChanged()),        this, SIGNAL(volumeChanged()));
            connect(concrete, SIGNAL(playbackStateChanged()), this, SIGNAL(playbackStateChanged()));
            connect(concrete, SIGNAL(mediaLoadedChanged()),   this, SIGNAL(mediaLoadedChanged()));
        }
    }

    static void inject(const std::shared_ptr<PlayerPresenter> &presenter) {
        s_injectedPresenter = presenter; // Ref count incremented, caller's instance unaffected
    }

    QString title()         const { return m_presenter ? m_presenter->title()       : QString(); }
    QString artist()        const { return m_presenter ? m_presenter->artist()      : QString(); }
    QString album()         const { return m_presenter ? m_presenter->album()       : QString(); }
    QUrl    cover()         const { return m_presenter ? m_presenter->cover()       : QUrl();    }
    quint64 duration_ms()   const { return m_presenter ? m_presenter->duration_ms() : 0;         }
    quint64 position_ms()   const { return m_presenter ? m_presenter->position_ms() : 0;         }
    quint8  volume()        const { return m_presenter ? m_presenter->volume()      : 0;         }
    bool    isMediaLoaded() const { return m_presenter && m_presenter->isMediaLoaded();          }

    PlaybackState playbackState() const {
        if (!m_presenter) return PlaybackState::stopped;

        using pp = PlayerPresenter::PlaybackState;
        switch (m_presenter->playbackState()) {
            case pp::paused:  return PlaybackState::paused;
            case pp::playing: return PlaybackState::playing;
            case pp::stopped: return PlaybackState::stopped;
        }
        Q_UNREACHABLE();
    }

    void setPosition_ms(quint64 position) { if (m_presenter) m_presenter->setPosition_ms(position); }
    void setVolume(quint8 volume)         { if (m_presenter) m_presenter->setVolume(volume); }

    Q_INVOKABLE void play()  const { if (m_presenter) m_presenter->play();  }
    Q_INVOKABLE void pause() const { if (m_presenter) m_presenter->pause(); }
    Q_INVOKABLE void stop()  const { if (m_presenter) m_presenter->stop();  }
    Q_INVOKABLE void next()  const { if (m_presenter) m_presenter->next();  }
    Q_INVOKABLE void prev()  const { if (m_presenter) m_presenter->prev();  }

    Q_INVOKABLE void notify_slider_pressed_change(bool pressed) {
        if (m_presenter) m_presenter->notify_slider_pressed_change(pressed);
    }

    Q_INVOKABLE void saveVolume() const { if (m_presenter) m_presenter->saveVolume(); }

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

private:
    std::shared_ptr<PlayerPresenter> m_presenter;
    inline static std::shared_ptr<PlayerPresenter> s_injectedPresenter = nullptr;
};