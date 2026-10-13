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
        PlayerPresenter.save_volume();
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
        playerRight: (stack_player.playerGlobalBounds.x + stack_player.playerGlobalBounds.width) / Math.max(1, root.width)
    }

    StackLayout {
        id: activeView

        anchors.fill: parent

        anchors.topMargin: currentIndex === 0 ? windexRow.height : 0

        Browser {
            id: browser
            onRequestTheaterMode: activeView.currentIndex = 1

            searchBar: searchBox
        }

        Player {
            id: stack_player

            background: bg
        }
    }

    RowLayout {
        id: navbar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        opacity: (activeView.currentIndex === 0 || !stack_player.immersive || navbar_hover.hovered) ? 1 : 0

        HoverHandler {
            id: navbar_hover
        }

        Item {
            Layout.preferredWidth: browser.vtabsColWidth - searchBox.leftPadding
        }

        RowLayout {
            id: windexRow

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            SearchBar {
                id: searchBox

                Layout.preferredWidth: width
                Layout.preferredHeight: height

                Layout.alignment: Qt.AlignVCenter

                onTextEdited: browser.activeVtab = 1 // Tracks

                // to still take space
                enabled: activeView.currentIndex === 0
                opacity: enabled ? 1 : 0
            }

            ResizableButton {
                text: qsTr("Clear")
                onClicked: {
                    searchBox.text = ""
                    searchBox.textEdited()
                }

                leftPadding: searchBox.leftPadding / 2
                rightPadding: searchBox.rightPadding / 2

                visible: searchBox.visible && searchBox.text !== ""

                filled: true

                magnify: false

                Layout.preferredHeight: searchBox.height
                Layout.alignment: Qt.AlignVCenter
            }
        }

        DragHandler {
            target: null
            
            onActiveChanged: {
                if (active) {
                    root.startSystemMove();
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Row {
            id: windex

            Layout.alignment: Qt.AlignVCenter

            property bool lightMode: PlayerPresenter.lumaSentinel && Light.lightModeForBounds(windexBounds, bg, PlayerPresenter) // context is everything

            ResizableButton {
                id: fullscreenToggle
                iconName: "arrow-left"

                visible: activeView.currentIndex === 1

                anchors.verticalCenter: parent.verticalCenter

                text: qsTr("Back")

                leftPadding: 10
                rightPadding: 10

                magnify: true

                lightMode: windex.lightMode

                onClicked: {
                    // if we are in the player
                    if (activeView.currentIndex === 1) {

                        // if it is in the lyrics view
                        if (stack_player.playerLyrics === 1) {
                            stack_player.playerLyrics = 0;
                        } else {
                            activeView.currentIndex = 0
                        }
                    }
                }
            }
            
            WindowDecorations {
                window: root

                Layout.fillWidth: true

                anchors.verticalCenter: parent.verticalCenter

                lightMode: windex.lightMode
            }
        }
    }

    SceneRect {
        id: windexBounds
        target: windex
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