pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.Presenter
import Player.Primitives
import Player.Fullscreen
import Player.LyricsManifest

ColumnLayout {
    id: root

    required property bool immersive // fed from Player
    required property Background background
    property PlayerPresenter stateModel: PlayerPresenter

    required property LyricsManifest model
    property int highlightedRowIndex: LyricsManifest.indexOfLyricToHighlight().row

    // whether to follow the lyrics by scrolling the list
    property bool autoscrollEnabled: true

    property real headerHeight: autoscrollEnabled ? Window.height / 2 : 0
    property real footerHeight: autoscrollEnabled ? Window.height / 2 : 0

    // also enable first and last rows to be centered too
    property bool centerEdgeLines: true

    signal switchToPlayerViewRequested ()

    signal clickedOnEmptySpace ()

    onClickedOnEmptySpace: {
        root.switchToPlayerViewRequested()
    }

    StackLayout {
        id: stack

        currentIndex: lyricsRows_c.count > 0 ? 1 : 0

        clip: true

        BigThinLabel {
            id: lyricsRows_p
            text: qsTr("No lyrics available.")

            color: Light.lightModeForBounds (lyricsRows_pBounds.bounds, root.background, root.stateModel)
                ? Theme.light.texts.unhighlightedLyric :
                  Theme.dark.texts.unhighlightedLyric

            TapHandler {
                onTapped: root.clickedOnEmptySpace()
            }
        }

        ListView {
            id: lyricsRows_c

            model: root.model

            readonly property bool lightMode: Light.lightModeForBounds(lyricsRows_cBounds.bounds, root.background, root.stateModel) // context is everything

            readonly property color highlightedColor:   lightMode ? Theme.light.texts.highlightedLyric   : Theme.dark.texts.highlightedLyric
            readonly property color unhighlightedColor: lightMode ? Theme.light.texts.unhighlightedLyric : Theme.dark.texts.unhighlightedLyric

            // Extra scroll room so row 0 / the last row can reach dead
            // center too, same as any line in the middle of the list.

            header: Item {
                height: root.headerHeight
                width: lyricsRows_c.width

                TapHandler {
                    onTapped: root.clickedOnEmptySpace()
                }
            }

            footer: Item {
                height: root.footerHeight
                width: lyricsRows_c.width

                TapHandler {
                    onTapped: root.clickedOnEmptySpace()
                }
            }

            function scrollToHighlighted() : void {
                const index = LyricsManifest.indexOfLyricToHighlight().row

                if (index >= count)
                    return

                root.highlightedRowIndex = index

                const fromY = contentY

                // No active line yet (position precedes the first timestamp):
                // rest at the very top, so the header space is what's on screen.
                if (index < 0)
                    positionViewAtBeginning()
                else
                    positionViewAtIndex(index, ListView.Center)

                const toY = contentY // center to screen
                contentY = fromY

                scrollAnimation.to = toY
                scrollAnimation.restart()
            }

            NumberAnimation {
                id: scrollAnimation
                target: lyricsRows_c
                property: "contentY"
                duration: 125
                easing.type: Easing.InOutCubic
            }
            
            readonly property bool doAutoscroll: !(moving || cooldownTimer.running || !root.autoscrollEnabled)

            onDoAutoscrollChanged: if (doAutoscroll) scrollToHighlighted()

            onMovingChanged: if (!moving) cooldownTimer.restart()

            Timer {
                id: cooldownTimer
                interval: 5000
                running: root.autoscrollEnabled
            }
            TapHandler {
                id: holdTracker
                target: null
                acceptedButtons: Qt.AllButtons
            }

            Connections {
                target: root
                function onHighlightedRowIndexChanged() {
                    if (lyricsRows_c.doAutoscroll)
                        lyricsRows_c.scrollToHighlighted()
                }
            }

            readonly property int interactiveWidth: root.width * 0.6
            readonly property int wrapWidth: root.width * 0.4
            
            delegate: Item {
                id: delegateRoot

                width: root.width
                height: delegateWidth.height

                required property lyric modelData
                required property int index

                TapHandler {
                    onTapped: (eventPoint, button) => {
                        // eventPoint.position is in delegateRoot coordinates
                        const onLyric = delegateWidth.contains(delegateWidth.mapFromItem(delegateRoot, eventPoint.position))

                        if (onLyric) {
                            root.stateModel.seekTo(del.model.timestamp)
                            lyricsRows_c.scrollToHighlighted()
                        } else {
                            root.clickedOnEmptySpace()
                        }
                    }
                }

                Item {
                    id: delegateWidth 

                    width: lyricsRows_c.interactiveWidth
                    height: del.height

                    anchors.horizontalCenter: parent.horizontalCenter

                    LyricDelegate {
                        id: del

                        model: delegateRoot.modelData

                        // Feeds only the break computation now — the item's
                        // actual on-screen width is implicit, so it's free to
                        // grow past this when highlighted.
                        wrapWidth: lyricsRows_c.wrapWidth
                        topPadding: highlighted ? 20 : 10
                        bottomPadding: topPadding

                        anchors.horizontalCenter: parent.horizontalCenter

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        unhighlightedColor: lyricsRows_c.unhighlightedColor
                        highlightedColor: lyricsRows_c.highlightedColor

                        highlighted: root.highlightedRowIndex === delegateRoot.index
                    }
                }
            }
        }

        SceneRect {
            id: lyricsRows_pBounds
            target: lyricsRows_p
        }

        SceneRect {
            id: lyricsRows_cBounds
            target: lyricsRows_c
        }
    }

    PlaybackControls {
        id: controls

        lightMode: Light.lightModeForBounds(controlsBounds.bounds, root.background, root.stateModel)

        visible: !root.immersive

        // free poll source
        pollWhileNotVisible: true
        onPosition_msChanged: root.highlightedRowIndex = LyricsManifest.indexOfLyricToHighlight().row

        Layout.maximumWidth: lyricsRows_c.wrapWidth
        Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
    }

    Connections {
        target: root.stateModel

        function onSeekFinished () {lyricsRows_c.scrollToHighlighted() } // this is not an autoscroll event
    }

    SceneRect {
        id: controlsBounds
        target: controls
    }
}