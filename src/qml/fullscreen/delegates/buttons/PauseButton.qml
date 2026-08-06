import QtQuick

import Player.Primitives
import Player.PlayerPresenter

ResizableButton {
    id: root

    icon.name: {
        switch (PlayerPresenter.playbackState) {
            case PlayerPresenter.PlaybackState.playing:
                return "media-playback-pause"
            case PlayerPresenter.PlaybackState.paused:
            case PlayerPresenter.PlaybackState.stopped:
            default:
                return "media-playback-start"
        }
    }

    onClicked: {
        if (PlayerPresenter.playbackState === PlayerPresenter.PlaybackState.playing) {
            PlayerPresenter.pause()
        } else {
            PlayerPresenter.play()
        }
    }
}