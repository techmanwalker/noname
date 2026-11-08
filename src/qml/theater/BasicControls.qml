import QtQuick

import Player.Theater

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
