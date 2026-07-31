import QtQuick
import QtQuick.Controls

import Player.Browser

ApplicationWindow {
    id: root

    visible: true
    title: "Noname - Browser"
    color: "#000"

    width: 950
    height: 650

    Browser {
        anchors.fill: parent
    }
}