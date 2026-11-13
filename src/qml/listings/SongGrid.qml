pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.MediaTypes
import Player.Primitives

GridView {
    id: root

    // model: the current folder model with members name, song
    property real songCoverWidth:  Theme.sizes.cardCoverWidth
    property real songCoverHeight: Theme.sizes.cardCoverHeight

    property int  scrollBarWidth:  Theme.sizes.scrollBarWidth

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

    // drag
    // The ghost spawner should handle excessively big selections by showing
    // "+N more…"
    property int maxDraggedSongs: Infinity

    // Tags this grid's drags, so a DropArea can tell them from other drags (file drops)
    readonly property string dragKey: "noname/tracks"

    // true from the long press that arms a drag until the pointer is released or the
    // drag is cancelled
    readonly property alias isDragging: selectionDrag.armed

    // true while tracks are really being dragged: from the first movement until AFTER the
    // drop was delivered (armed is already false by then, active is not)
    readonly property alias isCarrying: selectionDrag.active

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

        // song card
        MediaDelegate {
            property alias songItem: delegateRoot.modelData

            card: true

            title: songItem.title
            metadata: (songItem.artist.length > 0 ? songItem.artist : qsTr ("Unknown Artist"))
                + (songItem.album.length > 0 ? " · " + songItem.album : "")
            cover: songItem.cover
            duration: songItem.duration_mmss

            coverWidth: root.songCoverWidth
            coverHeight: root.songCoverHeight

            anchors.fill: parent
            anchors.leftMargin: LayoutMirroring.enabled ? root.songLateralPadding : 0
            anchors.rightMargin: LayoutMirroring.enabled ? 0 : root.songLateralPadding
            anchors.bottomMargin: root.songVerticalPadding

            // to visually mark it selected
            selected: selection.isSelected(delegateRoot.index)

            // lifted: the origin of what is being dragged
            opacity: selectionDrag.carries(delegateRoot.index) ? 0.4 : 1

            SelectionDrag.Arm   { controller: selectionDrag; row: delegateRoot.index }
            SelectionDrag.Track { controller: selectionDrag; row: delegateRoot.index }

            onClicked: {
                selection.clear() // a plain click drops the multi-selection
                root.songClicked(songItem)
            }
            onCtrlClicked: selection.toggle(delegateRoot.index)
            onRightClicked: selection.openMenuFor(delegateRoot.index)
        }
    }

    SelectionDrag {
        id: selectionDrag
        view: root
        selection: selection
        maxSelectedItems: root.maxDraggedSongs
        cancelSequences: [StandardKey.Cancel]
    }

    GhostSpawner {
        selectionDrag: selectionDrag
        cards: true
        dragKey: root.dragKey

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight
        cardColumnSpacing: root.songLateralPadding
        cardRowSpacing: root.songVerticalPadding
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
