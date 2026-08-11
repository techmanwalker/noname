pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player.App
import Player.Browser
import Player.Fullscreen
import Player.PlayerPresenter

ApplicationWindow {
    id: root

    visible: true
    title: "noname"
    color: "#000"

    width: WindowGeometry.width
    height: WindowGeometry.height

    flags: Qt.Window | Qt.FramelessWindowHint

    onClosing: {
        // save last used values
        WindowGeometry.save(width, height);
        PlayerPresenter.saveVolume();
    }

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

    StackLayout {
        id: activeView

        anchors.fill: parent

        Browser {
            id: browser
            onSwitchView: activeView.currentIndex = 1

            parentWindow: root
        }

        FullscreenPlayer {
            id: fullscreenPlayer
            onSwitchView: activeView.currentIndex = 0

            parentWindow: root
        }
    }


    // Resize handles for the first 20 pixels of each side
    
    component ResizeHandle: Item {
        id: handle
        
        property int edges
        property int cursorShape

        z: 100 // Guarantees handles sit above all UI elements

        HoverHandler {
            cursorShape: handle.cursorShape
        }

        DragHandler {
            onActiveChanged: {
                if (active) {
                    root.startSystemResize(handle.edges)
                }
            }
        }
    }

    // Top, Bottom, Left, Right edges
    ResizeHandle { anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: 20; anchors.rightMargin: 20; height: 20; edges: Qt.TopEdge; cursorShape: Qt.SizeVerCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: 20; anchors.rightMargin: 20; height: 20; edges: Qt.BottomEdge; cursorShape: Qt.SizeVerCursor }
    ResizeHandle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: 20; anchors.bottomMargin: 20; width: 20; edges: Qt.LeftEdge; cursorShape: Qt.SizeHorCursor }
    ResizeHandle { anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: 20; anchors.bottomMargin: 20; width: 20; edges: Qt.RightEdge; cursorShape: Qt.SizeHorCursor }

    // Four corners
    ResizeHandle { anchors.top: parent.top; anchors.left: parent.left; width: 20; height: 20; edges: Qt.TopEdge | Qt.LeftEdge; cursorShape: Qt.SizeFDiagCursor }
    ResizeHandle { anchors.top: parent.top; anchors.right: parent.right; width: 20; height: 20; edges: Qt.TopEdge | Qt.RightEdge; cursorShape: Qt.SizeBDiagCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.left: parent.left; width: 20; height: 20; edges: Qt.BottomEdge | Qt.LeftEdge; cursorShape: Qt.SizeBDiagCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.right: parent.right; width: 20; height: 20; edges: Qt.BottomEdge | Qt.RightEdge; cursorShape: Qt.SizeFDiagCursor }
}