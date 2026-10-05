import QtQuick
import QtQuick.Layouts

import Player.App
import Player.Fullscreen
import Player.LyricsManifest
import Player.Primitives
import Player.PlayerPresenter

Item {
    id: root

    property Window     parentWindow   // to inset window decorations
    property Background background // to adapt the contents to the current lighting

    property bool immersive: false // hide all controls and buttons, leave you only with the music

    signal switchView() // to other specific view, currently leaving empty means "switch to fullscreen player"

    // readonly, to set the background light anchors
    readonly property alias coverGlobalX: fsp.coverGlobalX
    readonly property alias coverSize: fsp.coverSize

    function lightModeForBounds (bounds: rect, name: string) : bool {

        // use the multiplier values background uses
        let multiplier = root.background.lumaMultiplierAtCenter (bounds);

        let m_backdrop = PlayerPresenter.backdropLumaForRect(bounds) * multiplier;
        let m_ring     = PlayerPresenter.ringLumaForRect(bounds)     * multiplier;

        // console.log ("name: " + name + " ; multiplier: " + multiplier + " ; multiplied backdrop luma: " + m_backdrop + " ; ring luma: " + m_ring)

        if (m_backdrop > 0.92) return true;   // always light
        if (m_backdrop < 0.05) return false;  // always dark (redundant while the gate below is 0.48)

        return m_backdrop > 0.7 && m_ring > 0.6;
    }

    StackLayout {
        id: stack

        anchors.fill: parent

        anchors.margins: 20 // leave space for the handles

        FullscreenPlayer {
            id: fsp

            background: root.background
            immersive: root.immersive

            onImmersiveChanged: {
                root.immersive = immersive
            }

            onSwitchToLyricsViewRequested: stack.currentIndex = 1
        }

        // just so I can see them
        Lyrics {
            id: lyrics

            model: LyricsManifest
            highlightedRowIndex: LyricsManifest.highlighted.row

            onSwitchToPlayerViewRequested: stack.currentIndex = 0

            property bool lightMode: PlayerPresenter.lumaSentinel && root.lightModeForBounds(lyricsBounds) // context is everything

            highlightedColor:   lightMode ? Theme.light.texts.highlightedLyric   : Theme.dark.texts.highlightedLyric
            unhighlightedColor: lightMode ? Theme.light.texts.unhighlightedLyric : Theme.dark.texts.unhighlightedLyric
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

            lightMode: PlayerPresenter.lumaSentinel && root.lightModeForBounds(windexBounds, "windex") // context is everything
        }
        
        ResizableButton {
            id: fullscreenToggle
            iconName: "arrow-left"

            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Back")

            padding: 20

            magnify: true

            lightMode: windex.lightMode

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

    SceneRect {
        id: windexBounds
        target: windex
    }

    SceneRect {
        id: lyricsBounds
        target: lyrics
    }
}