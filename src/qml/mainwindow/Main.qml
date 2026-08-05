import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player.Browser
import Player.Fullscreen
import Player.PlayerPresenter

ApplicationWindow {
    id: root

    visible: true
    title: "noname"
    color: "#000"

    width: 950
    height: 650

    flags: Qt.Window | Qt.FramelessWindowHint

    Background {
        source: activeView.currentIndex == 1 ? PlayerPresenter.cover : ""
        anchors.fill: parent

        // Math.max(1, root.width) prevents zero-division errors during
        // the brief moment when the window is still being constructed
        playerLeft:  0
        coverLeft:   fullscreenPlayer.coverGlobalX                                 / Math.max(1, root.width)
        coverRight:  (fullscreenPlayer.coverGlobalX + fullscreenPlayer.coverSize)  / Math.max(1, root.width)
        playerRight: 1
    }

    WindowDecorations {
        id: windex

        window: root

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
    }

    StackLayout {
        id: activeView

        anchors.top: windex.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

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