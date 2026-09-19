pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.PlayerPresenter
import Player.Primitives
import Player.Fullscreen
import Player.LyricsManifest

StackLayout {
    id: root

    required property LyricsManifest model
    required property int highlightedRowIndex

    // whether to follow the lyrics by scrolling the list
    property bool autoscrollEnabled: true

    property real headerHeight: root.autoscrollEnabled ? Window.height / 2 : 0
    property real footerHeight: root.autoscrollEnabled ? Window.height / 2 : 0

    property color unhighlightedColor: "#80ffffff"
    property color highlightedColor: "white"

    // also enable first and last rows to be centered too
    property bool centerEdgeLines: true

    currentIndex: listView.count > 0 ? 1 : 0
    
    signal switchToPlayerViewRequested ()

    Label {
        text: qsTr("No lyrics.")

        font.pointSize: 32
        font.weight: Font.Light

        opacity: 0.4

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment:   Text.AlignVCenter

        color: root.unhighlightedColor

        TapHandler {
            onTapped: root.switchToPlayerViewRequested()
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
        }

        footer: Item {
            height: root.footerHeight
            width: listView.width
        }

        function scrollToHighlighted() : void {
            const index = root.highlightedRowIndex
            if (index < 0 || index >= count)
                return

            const fromY = contentY
            positionViewAtIndex(index, ListView.Center)
            const toY = contentY
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

        onTopMarginChanged: if (doAutoscroll) scrollToHighlighted()

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
        
        delegate: Item {
            id: delegateRoot

            width: root.width
            height: del.height

            required property lyric modelData
            required property int index

            LyricDelegate {
                id: del

                model: delegateRoot.modelData

                // Feeds only the break computation now — the item's
                // actual on-screen width is implicit, so it's free to
                // grow past this when highlighted.
                wrapWidth: parent.width / 10 * 4
                topPadding: highlighted ? 20 : 10
                bottomPadding: topPadding

                anchors.horizontalCenter: parent.horizontalCenter

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                unhighlightedColor: root.unhighlightedColor
                highlightedColor: root.highlightedColor

                highlighted: root.highlightedRowIndex === delegateRoot.index
            }

            TapHandler {
                onTapped: PlayerPresenter.position_ms = del.model.timestamp
            }
        }
    }
}