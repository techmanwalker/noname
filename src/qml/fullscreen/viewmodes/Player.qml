import QtQuick
import QtQuick.Layouts

import Player.App
import Player.Fullscreen
import Player.LyricsManifest
import Player.Primitives

Item {
    id: root

    property Window parentWindow // to inset window decorations

    property bool immersive: false // hide all controls and buttons, leave you only with the music

    signal switchView() // to other specific view, currently leaving empty means "switch to fullscreen player"

    // readonly, to set the background light anchors
    readonly property alias coverGlobalX: fsp.coverGlobalX
    readonly property alias coverSize: fsp.coverSize

    property bool lightMode: false

    StackLayout {
        id: stack

        anchors.fill: parent

        anchors.margins: 20 // leave space for the handles

        FullscreenPlayer {
            id: fsp

            immersive: root.immersive

            onImmersiveChanged: {
                root.immersive = immersive
            }

            onSwitchToLyricsViewRequested: stack.currentIndex = 1

            lightMode: root.lightMode
        }

        // just so I can see them
        Lyrics {
            model: LyricsManifest
            highlightedRowIndex: LyricsManifest.highlighted.row

            onSwitchToPlayerViewRequested: stack.currentIndex = 0

            highlightedColor: root.lightMode ? "black" : "white"
            unhighlightedColor: root.lightMode ? "#80000000" : "#80ffffff"
        }
    }

    // ── Navigation ─────────────────────────────────────────────────────────

    Row {
        id: nav
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        opacity: 1

        states: [
            State {
                name: "hidden"

                PropertyChanges {
                    nav.opacity: 0
                }

                when: root.immersive && !windex_hover.hovered
            },

            State {
                name: "partiallyVisible"

                PropertyChanges {
                    nav.opacity: 0.4
                }

                when: !root.immersive && stack.currentIndex == 1 && !windex_hover.hovered
            }
        ]

        layoutDirection: Qt.RightToLeft

        spacing: windex.squareButtonWidth / 3 * 2

        HoverHandler {
            id: windex_hover
        }

        WindowDecorations {
            id: windex 
            window: root.parentWindow

            anchors.verticalCenter: parent.verticalCenter

            lightMode: root.lightMode
        }
        
        ResizableButton {
            id: fullscreenToggle
            iconName: "arrow-left"

            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Back")

            padding: 20

            magnify: true

            lightMode: root.lightMode

            onClicked: {
                // if it is in the lyrics view
                if (stack.currentIndex === 1) {
                    stack.currentIndex = 0;
                } else {
                    root.switchView()
                }
            }
        }

        DragHandler {
            target: null
            
            onActiveChanged: {
                if (active) {
                    root.parentWindow.startSystemMove();
                }
            }
        }
    }

}