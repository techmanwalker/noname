import QtQuick
import QtQuick.Controls

import Player.StartPage

ApplicationWindow {
    id: root

    visible: true
    title: "Noname - Startpage"
    color: "#000"

    width: 950
    height: 650

    StartPage {
        anchors.fill: parent
    }
}