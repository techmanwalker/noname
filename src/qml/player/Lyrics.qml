import QtQuick

import Player

Column {
    id: root
    spacing: 0

    width: implicitWidth
    height: implicitHeight

    required property var model
    
    Repeater {
        id: repeater

        model: root.model

        delegate: LyricsItemDelegate {
            required property var model

            text: model.text
            timestamp: model.timestamp

        }
    }
}