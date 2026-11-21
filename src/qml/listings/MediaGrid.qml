pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.MediaTypes
import Player.Primitives

GridView {
    id: root

    // model: the current folder model with members name, track
    property real trackCoverWidth:  Theme.sizes.cardCoverWidth
    property real trackCoverHeight: Theme.sizes.cardCoverHeight

    property int  scrollBarWidth:  Theme.sizes.scrollBarWidth

    property bool lightMode: false

    property real trackLateralPadding: (trackCoverWidth / 12)
    property real trackVerticalPadding: (trackCoverWidth / 12)

    cellWidth: dummycard.width + trackLateralPadding
    cellHeight: dummycard.height + trackVerticalPadding

    signal trackClicked(track clicked_track)

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
    property real maxDraggedTracks: Infinity

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
    rightMargin: scrollbar.logicalWidth - trackLateralPadding
    bottomMargin: -trackVerticalPadding

    delegate: Item {
        id: delegateRoot

        required property track modelData
        required property int index

        width: GridView.view.cellWidth
        height: GridView.view.cellHeight

        // track card
        MediaDelegate {
            property alias trackItem: delegateRoot.modelData

            card: true

            title: trackItem.title
            metadata: (trackItem.artist.length > 0 ? trackItem.artist : qsTr ("Unknown Artist"))
                + (trackItem.album.length > 0 ? " · " + trackItem.album : "")
            cover: trackItem.cover
            duration: trackItem.duration_mmss

            coverWidth: root.trackCoverWidth
            coverHeight: root.trackCoverHeight

            anchors.fill: parent
            anchors.leftMargin: LayoutMirroring.enabled ? root.trackLateralPadding : 0
            anchors.rightMargin: LayoutMirroring.enabled ? 0 : root.trackLateralPadding
            anchors.bottomMargin: root.trackVerticalPadding

            // to visually mark it selected
            selected: selection.isSelected(delegateRoot.index)

            // lifted: the origin of what is being dragged
            opacity: selectionDrag.carries(delegateRoot.index) ? 0.4 : 1

            SelectionDrag.Arm   { controller: selectionDrag; row: delegateRoot.index }
            SelectionDrag.Track { controller: selectionDrag; row: delegateRoot.index }

            onClicked: {
                selection.clear() // a plain click drops the multi-selection
                root.trackClicked(trackItem)
            }
            onCtrlClicked: selection.toggle(delegateRoot.index)
            onRightClicked: selection.openMenuFor(delegateRoot.index)
        }
    }

    SelectionDrag {
        id: selectionDrag
        view: root
        selection: selection
        maxSelectedItems: root.maxDraggedTracks
        cancelSequences: [StandardKey.Cancel]
    }

    GhostSpawner {
        selectionDrag: selectionDrag
        cards: true
        dragKey: root.dragKey
        lightMode: root.lightMode

        coverWidth: root.trackCoverWidth
        coverHeight: root.trackCoverHeight
        cardColumnSpacing: root.trackLateralPadding
        cardRowSpacing: root.trackVerticalPadding
    }

    TallestDummyCard {
        id: dummycard
        visible: false

        coverWidth: root.trackCoverWidth
        coverHeight: root.trackCoverHeight
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 12

        policy: ScrollBar.AsNeeded

        lightMode: root.lightMode
    }
}
