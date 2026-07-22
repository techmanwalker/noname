pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player
import Player.StartPage

ApplicationWindow {
    id: root

    visible: true
    title: "noname"
    color: "#000"

    width: 950
    height: 650

    Loader {
        id: activeView

        anchors.fill: parent
        sourceComponent: start
    }

    Component {
        id: start

        StartPage {
            onSwitchView: activeView.sourceComponent = player
        }
    }

    Component {
        id: player

        FullscreenPlayer {
            onSwitchView: activeView.sourceComponent = start
        }
    }
}