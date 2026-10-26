import QtQuick
import QtQuick.Layouts

import Player.Fullscreen
import Player.LyricsManifest
import Player.Primitives
import Player.Presenter

Item {
    id: root

    property Background background // to adapt the contents to the current lighting

    property bool immersive: false // hide all controls and buttons, leave you only with the music

    property alias playerLyrics: stack.currentIndex // 0 = player, 1 = lyrics

    // readonly, to set the background light anchors
    readonly property alias playerGlobalBounds: stackBounds.bounds
    readonly property alias coverGlobalBounds:  fsp.coverGlobalBounds
    readonly property alias coverSize: fsp.coverSize

    signal backToBrowser()
    signal backButtonClicked_r()

    onBackButtonClicked_r: {
        // if it is in the lyrics view
        if (playerLyrics === 1) {
            playerLyrics = 0;
        } else {
            backToBrowser()
        }
    }

    StackLayout {
        id: stack

        // anchors.margins: 20 // leave space for the handles

        anchors.fill: parent

        Item {
            TapHandler {
                onTapped: (eventPoint, button) => {
                    // eventPoint.position is in delegateRoot coordinates
                    const onPlayer = fsp.contains(fsp.mapFromItem(parent, eventPoint.position))

                    if (!onPlayer)
                        root.backToBrowser()
                }
            }

            FullscreenPlayer {
                id: fsp

                anchors.centerIn: parent

                background: root.background
                immersive: root.immersive
                availableHeight: root.height

                onImmersiveChanged: {
                    root.immersive = immersive
                }

                onSwitchToLyricsViewRequested: stack.currentIndex = 1
            }
        }
        
        Lyrics {
            id: lyrics

            model: LyricsManifest
            highlightedRowIndex: LyricsManifest.highlighted.row // qmllint disable

            onSwitchToPlayerViewRequested: stack.currentIndex = 0

            background: root.background
            immersive: fsp.immersive

            property bool lightMode: Light.lightModeForBounds(lyricsBounds, root.background, PlayerPresenter) // context is everything

            highlightedColor:   lightMode ? Theme.light.texts.highlightedLyric   : Theme.dark.texts.highlightedLyric
            unhighlightedColor: lightMode ? Theme.light.texts.unhighlightedLyric : Theme.dark.texts.unhighlightedLyric
        }
    }

    SceneRect {
        id: stackBounds
        target: stack
    }

    SceneRect {
        id: lyricsBounds
        target: lyrics
    }
}