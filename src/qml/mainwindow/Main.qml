pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts

import Player.App
import Player.Browser
import Player.Fullscreen
import Player.PlayerPresenter
import Player.Primitives

T.ApplicationWindow {
    id: root

    visible: true
    title: "noname"
    color: "#000"

    flags: Qt.Window | Qt.FramelessWindowHint

    width: WindowGeometry.width
    height: WindowGeometry.height

    onWidthChanged:  WindowGeometry.width  = width;
    onHeightChanged: WindowGeometry.height = height;

    onClosing: {
        // save last used values
        WindowGeometry.poll_and_save_to_disk(width, height);
        PlayerPresenter.saveVolume();
    }

    Column {
        // just so the values pop in console
        
        visible: false

        Label {
            text: PlayerPresenter.coverNuclearLuma
        }

        Label {
            text: PlayerPresenter.coverCentricLuma
        }

        Label {
            text: PlayerPresenter.coverMidringLuma
        }

        Label {
            text: PlayerPresenter.coverBordersLuma
        }

        Label {
            text: PlayerPresenter.coverPonderedLuma
        }
    }

    Background {
        id: bg
        source: activeView.currentIndex === 1 ? PlayerPresenter.cover : ""
        anchors.fill: parent

        maximumWidth: Screen.width
        maximumHeight: Screen.height

        contentLightness: PlayerPresenter.coverPonderedLuma /* from 0 to 1 */

        // Math.max(1, root.width) prevents zero-division errors during
        // the brief moment when the window is still being constructed
        playerLeft:   stack_player.playerGlobalBounds.x                                          / Math.max(1, root.width)
        coverLeft:    stack_player.coverGlobalBounds.x                                           / Math.max(1, root.width)
        coverRight:  (stack_player.coverGlobalBounds.x  + stack_player.coverGlobalBounds.width)  / Math.max(1, root.width)
        playerRight:  stack_player.playerGlobalBounds.x + stack_player.playerGlobalBounds.width  / Math.max(1, root.width)
    }

    StackLayout {
        id: activeView

        anchors.fill: parent

        Browser {
            id: browser
            onSwitchView: activeView.currentIndex = 1

            parentWindow: root
        }

        Player {
            id: stack_player
            onSwitchView: activeView.currentIndex = 0

            parentWindow: root

            background: bg
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
    /*
    ResizeHandle { anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: 20; anchors.rightMargin: 20; height: 20; edges: Qt.TopEdge; cursorShape: Qt.SizeVerCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: 20; anchors.rightMargin: 20; height: 20; edges: Qt.BottomEdge; cursorShape: Qt.SizeVerCursor }
    ResizeHandle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: 20; anchors.bottomMargin: 20; width: 20; edges: Qt.LeftEdge; cursorShape: Qt.SizeHorCursor }
    ResizeHandle { anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: 20; anchors.bottomMargin: 20; width: 20; edges: Qt.RightEdge; cursorShape: Qt.SizeHorCursor }
    */

    // Four corners
    ResizeHandle { anchors.top: parent.top; anchors.left: parent.left; width: 20; height: 20; edges: Qt.TopEdge | Qt.LeftEdge; cursorShape: Qt.SizeFDiagCursor }
    ResizeHandle { anchors.top: parent.top; anchors.right: parent.right; width: 20; height: 20; edges: Qt.TopEdge | Qt.RightEdge; cursorShape: Qt.SizeBDiagCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.left: parent.left; width: 20; height: 20; edges: Qt.BottomEdge | Qt.LeftEdge; cursorShape: Qt.SizeBDiagCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.right: parent.right; width: 20; height: 20; edges: Qt.BottomEdge | Qt.RightEdge; cursorShape: Qt.SizeFDiagCursor }
}