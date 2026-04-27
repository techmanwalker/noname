// Lyrics.qml
import QtQuick

import Player

Column {
    id: root
    spacing: 0

    width: implicitWidth
    height: implicitHeight
    
    Repeater {
        id: repeater

        model: LyricsModel // qmllint disable
        delegate: LyricsItemDelegate {} // qmllint disable
    }
}