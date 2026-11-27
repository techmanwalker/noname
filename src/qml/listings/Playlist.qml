pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.MediaTypes
import Player.PlayQueue
import Player.Primitives

AutoscrollListView {
    id: root

    property size rowSize: Qt.size (Math.max(width - rightMargin, 120), 80)

    property bool lightMode: false

    property bool trackLeftLightMode:  lightMode
    property bool trackRightLightMode: lightMode

    property int playheadRowIdx: -1

    // The playing row rests below the top edge by 0.4x its own height
    focusIndex: playheadRowIdx
    focusPosition: ListView.Beginning
    focusMarginRatio: 0.4
    autoscrollCooldownMs: Animations.scroll.playlist.cooldown
    autoscrollPaged: true

    // a drag never makes the ListView "move", so userInteracting is blind to it
    autoscrollEnabled: !mediaDrag.armed

    property int scrollBarWidth: Theme.sizes.scrollBarWidth

    // expose to Theater
    readonly property int trackCoverWidth:     Theme.sizes.rowTrackCoverWidth
    readonly property int trackCoverHeight:    Theme.sizes.rowTrackCoverWidth
    readonly property int trackInnerSpacing:   trackCoverWidth / 8
    readonly property int trackFadePadding:    Theme.sizes.rowTrackFadePadding
    readonly property int trackLateralPadding: Theme.sizes.rowTrackLateralPadding

    // Right padding reserves space for the scrollbar so it appears
    // to float outside the list content without overlapping it
    rightMargin: scrollbar.logicalWidth

    signal trackClicked(track clicked_track)

    // This list only reports the gesture; committing the move is the consumer's job.
    signal moveRequested(int from, int to)

    // Most rows a drag carries. 1: a lone row is a reorder, as in the queue.
    property real maxDraggedTracks: 1

    // Tags this list's drags for a DropArea. Empty: nothing takes them.
    property string dragKey

    readonly property alias isDragging: mediaDrag.armed
    readonly property alias isCarrying: mediaDrag.active

    // selection
    property alias selectedIndices: mediaSelection.indices
    property alias additionalMenuActions: mediaSelection.extraActions
    property alias trackAt: mediaSelection.trackAt

    MediaSelection {
        id: mediaSelection
        model: root.model
    }

    SelectionDrag {
        id: mediaDrag
        view: root
        selection: mediaSelection
        maxSelectedItems: root.maxDraggedTracks
        cancelSequences: root.dragKey !== "" ? [StandardKey.Cancel] : []
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
        dragKey: root.dragKey

        // a reorder inside this list is taken by the list itself, no DropArea answers it
        acceptsDrop: rowMove.willMove

        lightMode: root.lightMode
        leftLightMode:  root.trackLeftLightMode
        rightLightMode: root.trackRightLightMode

        rowWidth: root.width - root.rightMargin

        coverWidth: root.trackCoverWidth
        coverHeight: root.trackCoverHeight
    }

    // A glide is never restarted mid-way; it finishes and then re-aims (see onFinished below)
    onFocusFollowed: if (!playheadGlide.running) playheadGlide.start()

    MediaDelegateBackground {
        id: playheadBackground

        // Not a content item by default: parent it there so it scrolls with the rows
        parent: root.contentItem

        width:  root.rowSize.width
        height: root.rowSize.height

        // Rides the playhead row in content space. The focus margin brings the row to
        // the pin; where the scroll can't (cooldown, drag, clamped ends) it stays on the row.
        visible: root.playheadRowIdx >= 0

        // Where the highlight is, in rows. It glides toward the playhead row, so everything
        // below derives its pixel position from the list's real geometry.
        property real row: 0

        // Not row * stride: the ListView lays out lazily from the first visible row, so rows
        // removed above the viewport move originY instead of the visible rows.
        y: root.originY + (root.headerItem ? root.headerItem.height : 0)
           + row * (root.rowSize.height + root.spacing)

        Component.onCompleted: row = Math.max(root.playheadRowIdx, 0)

        // Same transition as the scroll, started by the same call (focusFollowed). While the
        // scroll carries the row to the pin, both move by the same amount and the highlight is still.
        NumberAnimation {
            id: playheadGlide
            target: playheadBackground
            property: "row"
            to: Math.max(root.playheadRowIdx, 0)
            duration: root.autoscrollDurationMs
            easing.type: root.autoscrollEasing

            onFinished: if (playheadBackground.row !== to) restart()
        }

        playing: true
        lightMode: root.lightMode

        inner_leftstop:   Theme.sizes.rowTrackFadePadding                       / Math.max(1, root.width)
        inner_rightstop: (root.rowSize.width - Theme.sizes.rowTrackFadePadding) / Math.max(1, root.width)
    }

    delegate: MediaDelegate {
        id: track

        required property track modelData
        required property int index

        title:      modelData.title
        cover:      modelData.cover
        metadata:   (modelData.artist.length > 0 ? modelData.artist : qsTr ("Unknown Artist"))
            + (modelData.album.length > 0 ? " · " + modelData.album : "")
        duration:   modelData.duration_mmss

        width:  root.rowSize.width
        height: root.rowSize.height

        lightMode:      root.lightMode
        leftLightMode:  root.trackLeftLightMode
        rightLightMode: root.trackRightLightMode

        showSeparator: true

        clip: true

        selected: mediaSelection.isSelected(index)

        onSelectedChanged: console.log("index " + index + " is now " + (selected ? "selected" : "not selected"))

        opacity: mediaDrag.carries(index) ? 0.4 : 1

        SelectionDrag.Arm   { controller: mediaDrag; row: track.index }
        SelectionDrag.Track { controller: mediaDrag; row: track.index }

        onClicked: {
            mediaSelection.clear() // a plain click drops the multi-selection
            root.trackClicked(modelData)
        }
        onCtrlClicked: mediaSelection.toggle(index)
        onRightClicked: mediaSelection.openMenuFor(index)
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 3

        policy: ScrollBar.AsNeeded

        lightMode: root.trackRightLightMode
    }
}
