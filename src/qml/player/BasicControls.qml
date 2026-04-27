import QtQuick
import QtQuick.Controls

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