import QtQuick
import QtQuick.Controls

import Player
import Player.MediaSequences

ApplicationWindow {
    id: root

    visible: true
    title: "Noname - Startpage"
    color: "#000"

    width: 950
    height: 650

    Item {
        anchors.fill: parent
        
        Playlist {
            anchors.fill: parent
            model: ShortcutsList
        }
    }
}