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
    required property int highlightedRowIndex

    // whether to follow the lyrics by scrolling the list
    property bool autoscrollEnabled: true

    property real headerHeight: autoscrollEnabled ? Window.height / 2 : 0
    property real footerHeight: autoscrollEnabled ? Window.height / 2 : 0

    property color unhighlightedColor: Theme.dark.texts.unhighlightedLyric
    property color highlightedColor: Theme.dark.texts.highlightedLyric

    // also enable first and last rows to be centered too
    property bool centerEdgeLines: true

    signal switchToPlayerViewRequested ()

    signal clickedOnEmptySpace ()

    onClickedOnEmptySpace: {
        root.switchToPlayerViewRequested()
    }

    StackLayout {
        id: stack

        currentIndex: listView.count > 0 ? 1 : 0

        clip: true

        BigThinLabel {
            text: qsTr("No lyrics available.")

            TapHandler {
                onTapped: root.clickedOnEmptySpace()
            }
        }

        ListView {
            id: listView

            model: root.model

            // Extra scroll room so row 0 / the last row can reach dead
            // center too, same as any line in the middle of the list.

            header: Item {
                height: root.headerHeight
                width: listView.width

                TapHandler {
                    onTapped: root.clickedOnEmptySpace()
                }
            }

            footer: Item {
                height: root.footerHeight
                width: listView.width

                TapHandler {
                    onTapped: root.clickedOnEmptySpace()
                }
            }

            function scrollToHighlighted() : void {
                const index = root.highlightedRowIndex

                if (index >= count)
                    return

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
                target: listView
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
                    if (listView.doAutoscroll)
                        listView.scrollToHighlighted()
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

                        if (onLyric)
                            root.stateModel.position_ms = del.model.timestamp
                        else
                            root.clickedOnEmptySpace()
                    }
                }

                Item {
                    id: delegateWidth 

                    width: listView.interactiveWidth
                    height: del.height

                    anchors.horizontalCenter: parent.horizontalCenter

                    LyricDelegate {
                        id: del

                        model: delegateRoot.modelData

                        // Feeds only the break computation now — the item's
                        // actual on-screen width is implicit, so it's free to
                        // grow past this when highlighted.
                        wrapWidth: listView.wrapWidth
                        topPadding: highlighted ? 20 : 10
                        bottomPadding: topPadding

                        anchors.horizontalCenter: parent.horizontalCenter

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        unhighlightedColor: root.unhighlightedColor
                        highlightedColor: root.highlightedColor

                        highlighted: root.highlightedRowIndex === delegateRoot.index
                    }
                }
            }
        }
    }

    PlaybackControls {
        id: controls

        lightMode: Light.lightModeForBounds(controlsBounds.bounds, root.background, root.stateModel)

        visible: !root.immersive

        Layout.maximumWidth: listView.wrapWidth
        Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
    }

    Connections {
        target: root.stateModel

        function onSeekFinished () {listView.scrollToHighlighted() } // this is not an autoscroll event
    }

    SceneRect {
        id: controlsBounds
        target: controls
    }
}