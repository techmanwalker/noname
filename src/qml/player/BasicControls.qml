import QtQuick

import Player

Row {
    id: root
    property bool paused: true

    SkipBackward {
    }

    PauseButton {
        paused: paused
    }

    SkipForward {
    }
}