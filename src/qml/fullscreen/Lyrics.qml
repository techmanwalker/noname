import QtQuick

import Player.Fullscreen

ListView {
    id: root

    delegate: LyricDelegate {
        required property var model

        timestamp: model.timestamp
        text: model.text
    }
}