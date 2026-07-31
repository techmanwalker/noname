import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player.Browser
import Player.Fullscreen

ApplicationWindow {
    id: root

    visible: true
    title: "noname"
    color: "#000"

    width: 950
    height: 650

    StackLayout {
        id: activeView
        anchors.fill: parent

        Browser {
            id: browser
            onSwitchView: activeView.currentIndex = 1
        }

        FullscreenPlayer {
            id: fullscreenPlayer
            onSwitchView: activeView.currentIndex = 0
        }
    }
}