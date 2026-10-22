import QtQuick

import Player.Primitives
import Player.Presenter

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

    onClicked: PlayerPresenter.togglePlayPause()
}