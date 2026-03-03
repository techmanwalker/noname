// Lyrics.qml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import "delegates"

ColumnLayout {
    id: root
    Layout.fillWidth: true
    
    spacing: 0

    height: implicitHeight
    
    Repeater {
        id: repeater
        model: lyricsModel // qmllint disable
        delegate: LyricsDelegate {}
    }
}