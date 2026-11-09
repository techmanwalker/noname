pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.MediaTypes
import Player.Primitives
import Player.PlayQueue

GridView {
    id: root

    // model: the current folder model with members name, song
    property real songCoverWidth: 72
    property real songCoverHeight: songCoverWidth

    property int scrollBarWidth: 4

    property bool lightMode: false

    property real songLateralPadding: (songCoverWidth / 12)
    property real songVerticalPadding: (songCoverWidth / 12)

    cellWidth: dummycard.width + songLateralPadding
    cellHeight: dummycard.height + songVerticalPadding

    signal songClicked(song clicked_song)

    // selection
    property alias selectedIndices: selection.indices
    property alias additionalMenuActions: selection.extraActions

    MediaSelection {
        id: selection
        model: root.model
    }

    clip: true

    // Reduce frame drops while scrolling
    displayMarginBeginning: 1000
    displayMarginEnd: 1000

    // Every cell keeps its padding so cards stay uniform; the trailing gutter of
    // the last column/row is cancelled here instead of in the delegate.
    rightMargin: scrollbar.logicalWidth - songLateralPadding
    bottomMargin: -songVerticalPadding

    delegate: Item {
        id: delegateRoot

        required property song modelData
        required property int index

        width: GridView.view.cellWidth
        height: GridView.view.cellHeight

        Song {
            property song songItem: delegateRoot.modelData

            readonly property alias index: delegateRoot.index

            card: true

            anchors.fill: parent
            anchors.leftMargin: LayoutMirroring.enabled ? root.songLateralPadding : 0
            anchors.rightMargin: LayoutMirroring.enabled ? 0 : root.songLateralPadding
            anchors.bottomMargin: root.songVerticalPadding

            title: songItem.title
            metadata: (songItem.artist.length > 0 ? songItem.artist : qsTr ("Unknown Artist"))
                + (songItem.album.length > 0 ? " · " + songItem.album : "")
            cover: songItem.cover
            duration: songItem.duration_mmss

            coverWidth: root.songCoverWidth
            coverHeight: root.songCoverHeight

            // to visually mark it selected
            selected: selection.isSelected(index)

            onClicked: {
                selection.clear() // a plain click drops the multi-selection
                root.songClicked(songItem)
            }
            onCtrlClicked: selection.toggle(index)
            onRightClicked: selection.openMenuFor(index)
        }
    }

    TallestDummyCard {
        id: dummycard
        visible: false

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 12

        policy: ScrollBar.AsNeeded

        lightMode: root.lightMode
    }
}
