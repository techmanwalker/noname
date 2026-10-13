import QtQuick
import QtQuick.Layouts

import Player.App
import Player.Fullscreen
import Player.LyricsManifest
import Player.Primitives
import Player.PlayerPresenter

Item {
    id: root

    property Background background // to adapt the contents to the current lighting

    property bool immersive: false // hide all controls and buttons, leave you only with the music

    property alias playerLyrics: stack.currentIndex // 0 = player, 1 = lyrics

    // readonly, to set the background light anchors
    readonly property alias playerGlobalBounds: stackBounds.bounds
    readonly property alias coverGlobalBounds:  fsp.coverGlobalBounds
    readonly property alias coverSize: fsp.coverSize

    function lightModeForBounds (bounds: rect, name: string) : bool {

        // use the multiplier values background uses
        let multiplier = root.background.lumaMultiplierAtCenter (bounds);

        let m_backdrop = PlayerPresenter.backdrop_luma_for_rect(bounds) * multiplier;
        let m_ring     = PlayerPresenter.ring_luma_for_rect(bounds)     * multiplier;

        // console.log ("name: " + name + " ; multiplier: " + multiplier + " ; multiplied backdrop luma: " + m_backdrop + " ; ring luma: " + m_ring)

        if (m_backdrop > 0.92) return true;   // always light
        if (m_backdrop < 0.05) return false;  // always dark (redundant while the gate below is 0.48)

        return m_backdrop > 0.7 && m_ring > 0.6;
    }

    StackLayout {
        id: stack

        // anchors.margins: 20 // leave space for the handles

        anchors.fill: parent

        Item {
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
            highlightedRowIndex: LyricsManifest.highlighted.row

            onSwitchToPlayerViewRequested: stack.currentIndex = 0

            property bool lightMode: PlayerPresenter.lumaSentinel && root.lightModeForBounds(lyricsBounds) // context is everything

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