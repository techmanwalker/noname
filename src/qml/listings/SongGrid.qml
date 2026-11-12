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

    // drag
    // The ghost grid is one card per carried track, so this is what keeps it from
    // growing to the size of the window.
    property int maxDraggedSongs: 9

    // Tags this grid's drags, so a DropArea can tell them from other drags (file drops)
    readonly property string dragKey: "noname/tracks"

    // true from the long press that arms a drag until the pointer is released or the
    // drag is cancelled
    readonly property alias isDragging: selectionDrag.armed

    SelectionDrag {
        id: selectionDrag
        view: root
        selection: selection
        maxSelectedItems: root.maxDraggedSongs
        cancelSequences: [StandardKey.Cancel]

        // the target of the drop (a DropArea listening for dragKey) takes `sources`
        onReleased: ghosts.Drag.drop()
    }

    clip: true

    // Reduce frame drops while scrolling
    displayMarginBeginning: 1000
    displayMarginEnd: 1000

    // Every cell keeps its padding so cards stay uniform; the trailing gutter of
    // the last column/row is cancelled here instead of in the delegate.
    rightMargin: scrollbar.logicalWidth - songLateralPadding
    bottomMargin: -songVerticalPadding

    // A song card. The delegate and the drag ghosts are both made from it, so a ghost
    // can't drift from what it was picked up from.
    component Card: Song {
        required property song songItem

        card: true

        title: songItem.title
        metadata: (songItem.artist.length > 0 ? songItem.artist : qsTr ("Unknown Artist"))
            + (songItem.album.length > 0 ? " · " + songItem.album : "")
        cover: songItem.cover
        duration: songItem.duration_mmss

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight
    }

    delegate: Item {
        id: delegateRoot

        required property song modelData
        required property int index

        width: GridView.view.cellWidth
        height: GridView.view.cellHeight

        Card {
            songItem: delegateRoot.modelData

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

    // What the drag carries, under the pointer: N x N cards, enough of them to fit the
    // tracks, in selection order, centered on the pointer.
    Grid {
        id: ghosts

        // The window's root, not the view: the view clips, and the ghosts have to leave
        // it to reach the drop target. Scene coordinates are then this item's own.
        // Asked of the view, not of this item: once re-parented to nothing, this item
        // would have no window to ask.
        parent: root.Window.contentItem
        z: 1000

        visible: selectionDrag.active
        enabled: false // never takes input
        opacity: 0.4

        columns: Math.max(1, Math.ceil(Math.sqrt(selectionDrag.items.length)))
        columnSpacing: root.songLateralPadding
        rowSpacing: root.songVerticalPadding

        x: selectionDrag.pointer.x - width / 2
        y: selectionDrag.pointer.y - height / 2

        // The same Drag a MouseArea would run: the pointer is its hot spot, so the drop
        // lands where the cursor is, whatever the size of the grid.
        Drag.active: selectionDrag.active
        Drag.keys: [root.dragKey]
        Drag.source: selectionDrag
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2

        Repeater {
            // built when armed, so they are there when the pointer starts moving
            model: selectionDrag.armed ? selectionDrag.items : []

            Card {
                required property song modelData

                songItem: modelData

                width: dummycard.width
                height: dummycard.height

                selected: true
            }
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
