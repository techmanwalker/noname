import QtQuick

import QtMultimedia
import Player.Primitives
import Player.PlayerPresenter

ResizableButton {
    id: root

    // Strictly real to the real playback state
    icon.name: {
        switch (PlayerPresenter.playbackState) {
            case MediaPlayer.PlayingState:
                return "media-playback-pause"
            case MediaPlayer.PausedState:
            case MediaPlayer.StoppedState:
            default:
                return "media-playback-start"
        }
    }

    onClicked: {
        if (PlayerPresenter.playbackState === MediaPlayer.PlayingState) {
            PlayerPresenter.pause()
        } else {
            PlayerPresenter.play()
        }
    }
}