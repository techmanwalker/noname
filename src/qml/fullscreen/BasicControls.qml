import QtQuick

import Player.Fullscreen

Row {
    id: root

    property bool lightMode: false

    SkipBackward {
        lightMode: root.lightMode
    }

    PauseButton {
        lightMode: root.lightMode
    }

    SkipForward {
        lightMode: root.lightMode
    }
}