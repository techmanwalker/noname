import QtQuick
import QtQuick.Controls

import PlayerModels

ToolButton {
    id: root

    // Strictly real to the real playback state
    icon.name: {
        switch (PlaybackPresentation.playbackState) {
            case PlaybackPresentation.PlayingState:
                return "media-playback-pause"
            case PlaybackPresentation.PausedState:
                return "media-playback-start"
            case PlaybackPresentation.StoppedState:
            default:
                return "media-playback-start" // O un icono de stop/play deshabilitado si prefieres
        }
    }

    // Opcional: Podrías deshabilitar el botón si está en Stopped y no hay pista cargada
    enabled: PlaybackPresentation.playbackState !== PlaybackPresentation.StoppedState || PlaybackPresentation.status === PlaybackPresentation.Ready

    onClicked: {
        if (PlaybackPresentation.playbackState === PlaybackPresentation.PlayingState) {
            PlaybackPresentation.pause()
        } else {
            PlaybackPresentation.play()
        }
    }
}