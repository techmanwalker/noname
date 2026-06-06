import QtQuick
import QtQuick.Controls

import QtMultimedia
import Player.PlaybackPresentation

ToolButton {
    id: root

    // Strictly real to the real playback state
    icon.name: {
        switch (PlaybackPresentation.playbackState) {
            case MediaPlayer.PlayingState:
                return "media-playback-pause"
            case MediaPlayer.PausedState:
            case MediaPlayer.StoppedState:
            default:
                return "media-playback-start"
        }
    }

    onClicked: {
        if (PlaybackPresentation.playbackState === MediaPlayer.PlayingState) {
            PlaybackPresentation.pause()
        } else {
            PlaybackPresentation.play()
        }
    }
}