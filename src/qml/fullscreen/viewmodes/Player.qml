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
        }

        // just so I can see them
        Lyrics {
            model: LyricsManifest
        }
    }

    // ── Navigation ─────────────────────────────────────────────────────────

    Row {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        anchors.topMargin: 20 // leave free space for window resize handle
        anchors.rightMargin: 20

        opacity: (!root.immersive || windex_hover.hovered) ? 1 : 0 // immersion

        layoutDirection: Qt.RightToLeft

        spacing: windex.squareButtonWidth / 3 * 2

        HoverHandler {
            id: windex_hover
        }

        WindowDecorations {
            id: windex 
            window: root.parentWindow

            anchors.verticalCenter: parent.verticalCenter
        }
        
        ResizableButton {
            id: fullscreenToggle
            iconName: "arrow-left"

            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Back")

            padding: 20

            magnify: true

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