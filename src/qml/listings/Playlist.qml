pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.MediaTypes
import Player.PlayQueue
import Player.Primitives

AutoscrollListView {
    id: root

    property bool lightMode: false

    property int playheadRowIdx: -1

    // The playing row rests below the top edge by 0.4x its own height
    focusIndex: playheadRowIdx
    focusPosition: ListView.Beginning
    focusMarginRatio: 0.4
    autoscrollCooldownMs: Animations.scroll.playlist.cooldown

    // a drag never makes the ListView "move", so userInteracting is blind to it
    autoscrollEnabled: !mediaDrag.armed

    property int scrollBarWidth: Theme.sizes.scrollBarWidth

    // expose to Theater
    readonly property int songCoverWidth:     Theme.sizes.rowTrackCoverWidth
    readonly property int songCoverHeight:    Theme.sizes.rowTrackCoverWidth
    readonly property int songInnerSpacing:   songCoverWidth / 8
    readonly property int songFadePadding:    Theme.sizes.rowTrackFadePadding
    readonly property int songLateralPadding: Theme.sizes.rowTrackLateralPadding

    // Right padding reserves space for the scrollbar so it appears
    // to float outside the list content without overlapping it
    rightMargin: scrollbar.logicalWidth

    signal songClicked(song clicked_song)

    // This list only reports the gesture; committing the move is the consumer's job.
    signal moveRequested(int from, int to)

    // selection
    property alias selectedIndices: mediaSelection.indices
    property alias additionalMenuActions: mediaSelection.extraActions
    property alias songAt: mediaSelection.songAt

    MediaSelection {
        id: mediaSelection
        model: root.model
    }

    SelectionDrag {
        id: mediaDrag
        view: root
        selection: mediaSelection
        maxSelectedItems: 1
    }

    // A lone dragged row is a move; any other number of rows is not
    SingleRowMove {
        id: rowMove
        view: root
        selectionDrag: mediaDrag
        lightMode: root.lightMode

        onMoveRequested: (from, to) => {
            mediaSelection.clear() // selected rows are raw ints, stale after a move
            root.moveRequested(from, to)
        }
    }

    // Several dragged rows travel as a ghost playlist. A lone row is SingleRowMove's, and
    // the Loader below already draws it.
    GhostSpawner {
        selectionDrag: mediaDrag
        cards: false

        // a reorder inside this list is taken by the list itself, no DropArea answers it
        acceptsDrop: rowMove.willMove

        lightMode: root.lightMode
        rowWidth: root.width - root.rightMargin

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight
    }

    delegate: MediaDelegate {
        id: song

        required property song modelData
        required property int index
        
        title:      modelData.title
        cover:      modelData.cover
        metadata:   (modelData.artist.length > 0 ? modelData.artist : qsTr ("Unknown Artist"))
            + (modelData.album.length > 0 ? " · " + modelData.album : "")
        duration:   modelData.duration_mmss

        width: root.width - root.rightMargin

        height: 80

        lightMode: root.lightMode

        // findBySource returns a different object from memory, hence comparing .row instead
        playing: playheadRowIdx >= 0 // qmllint disable
            && index === playheadRowIdx // qmllint disable

        showSeparator: true

        clip: true

        selected: mediaSelection.isSelected(index)

        opacity: mediaDrag.carries(index) ? 0.4 : 1

        SelectionDrag.Arm   { controller: mediaDrag; row: song.index }
        SelectionDrag.Track { controller: mediaDrag; row: song.index }

        onClicked: {
            mediaSelection.clear() // a plain click drops the multi-selection
            root.songClicked(modelData)
        }
        onCtrlClicked: mediaSelection.toggle(index)
        onRightClicked: mediaSelection.openMenuFor(index)
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 3

        policy: ScrollBar.AsNeeded

        lightMode: root.lightMode
    }
}
