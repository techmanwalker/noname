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

    // a DragHandler drag never makes the ListView "move", so userInteracting is blind to it
    autoscrollEnabled: dragFromRow < 0

    property int songCoverWidth:  48
    property int songCoverHeight: songCoverWidth
    property int scrollBarWidth: 4

    // expose to Theater
    property real songInnerSpacing: 8
    property real songFadePadding: 10
    property real songLateralPadding: 15

    // Right padding reserves space for the scrollbar so it appears
    // to float outside the list content without overlapping it
    rightMargin: scrollbar.logicalWidth

    signal songClicked(song clicked_song)

    // This list only reports the gesture; committing the move is the consumer's job.
    // Leave `reorderable` false when the model can't move items: rows aren't draggable at all.
    property bool reorderable: false
    signal moveRequested(int from, int to)

    // gesture state lives here, not in the delegate: delegates are pooled/recreated
    // dragFromRow >= 0 from the moment a long press arms the drag until the release
    property int dragFromRow: -1
    property int dragOverRow: -1

    // drag ghost
    property bool dragActive: false // the pointer is really dragging (armed AND moved)
    property song dragSong          // snapshot of the grabbed row, taken when the long press arms
    property point dragGhostPos     // ghost's top-left, in viewport coordinates

    // an armed press belongs to the drag: the Flickable must not steal it
    interactive: dragFromRow < 0

    // Ends the gesture, whichever handler notices the release first. Idempotent.
    // State is reset BEFORE emitting: re-enabling autoscroll restarts its cooldown, so the
    // playhead-row change caused by the move is deferred instead of yanking the view.
    function finishReorder(): void {
        const from = dragFromRow
        const to = dragOverRow
        dragFromRow = -1
        dragOverRow = -1

        if (from >= 0 && to >= 0 && to !== from) {
            selection.clear() // selected rows are raw ints, stale after a move
            moveRequested(from, to)
        }
    }

    // selection
    property alias selectedIndices: selection.indices
    property alias additionalMenuActions: selection.extraActions

    MediaSelection {
        id: selection
        model: root.model
    }

    // Drop marker. Moving down lands below the target row, moving up lands above it,
    // matching move_item's "ends up at dest's row" semantics.
    Rectangle {
        parent: root // overlay on the viewport, not inside the scrolling content
        z: 10

        readonly property Item target: root.dragOverRow >= 0 ? root.itemAtIndex(root.dragOverRow) : null

        visible: target !== null && root.dragOverRow !== root.dragFromRow
        width: root.width - root.rightMargin
        height: 2
        y: target
            ? target.y - root.contentY + (root.dragFromRow < root.dragOverRow ? target.height : 0) - height / 2
            : 0

        color: root.lightMode ? "black" : "white" // swap for your Theme color
    }

    // The grabbed row, following the pointer. It paints as selected so it reads as lifted
    // above the rows it crosses.
    Song {
        parent: root // viewport overlay, not scrolling content
        z: 20
        visible: root.dragActive
        enabled: false // never takes input

        x: root.dragGhostPos.x
        y: root.dragGhostPos.y
        width: root.width - root.rightMargin
        height: 80

        selected: true
        lightMode: root.lightMode

        title:    root.dragSong.title
        cover:    root.dragSong.cover
        metadata: (root.dragSong.artist.length > 0 ? root.dragSong.artist : qsTr ("Unknown Artist"))
            + (root.dragSong.album.length > 0 ? " · " + root.dragSong.album : "")
        duration: root.dragSong.duration_mmss

        coverWidth:     root.songCoverWidth
        innerSpacing:   root.songInnerSpacing
        fadePadding:    root.songFadePadding
        lateralPadding: root.songLateralPadding
    }

    delegate: Song {
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

        coverWidth: root.songCoverWidth

        innerSpacing:   root.songInnerSpacing
        fadePadding:    root.songFadePadding
        lateralPadding: root.songLateralPadding

        // findBySource returns a different object from memory, hence comparing .row instead
        playing: playheadRowIdx >= 0 // qmllint disable
            && index === playheadRowIdx // qmllint disable

        showSeparator: true

        clip: true

        selected: selection.isSelected(index)

        opacity: root.dragFromRow === index ? 0.4 : 1

        // Arms the drag. Moving past the drag threshold before the timer fires cancels this
        // handler, so a scroll sweep never arms.
        TapHandler {
            enabled: root.reorderable
            acceptedButtons: Qt.LeftButton
            acceptedModifiers: Qt.NoModifier
            longPressThreshold: Animations.dragThreshold / 1000 // seconds

            onLongPressed: {
                root.dragSong = song.modelData  // snapshot for the ghost
                root.dragFromRow = song.index   // dims the row, pauses autoscroll, locks the Flickable
            }

            // Ends an armed press that never became a drag. When it does become one, the
            // DragHandler (threshold 0) is already active before this handler gives up at the
            // system drag distance, so this must leave the gesture alone.
            onPressedChanged: if (!pressed && !reorderDrag.active) root.finishReorder()
        }

        DragHandler {
            id: reorderDrag
            enabled: root.reorderable
            target: null // the row stays where the ListView put it; only the gesture is tracked

            // Unarmed, no axis is draggable, so a sweep can't start this handler and stays the
            // Flickable's. Armed, the first movement starts the drag.
            xAxis.enabled: root.dragFromRow === song.index
            yAxis.enabled: root.dragFromRow === song.index
            dragThreshold: 0

            onActiveChanged: {
                root.dragActive = active
                if (!active) root.finishReorder()
            }

            onCentroidChanged: {
                if (!active) return
                const p = root.mapFromItem(null, centroid.scenePosition.x, centroid.scenePosition.y)
                // keep the grab offset, so the row doesn't jump to the pointer when the drag starts
                root.dragGhostPos = Qt.point(p.x - centroid.pressPosition.x, p.y - centroid.pressPosition.y)
                const y = root.contentY + p.y
                const row = root.indexAt(root.contentX, y)
                // below the last row is a legal drop: it means "to the end"
                root.dragOverRow = row >= 0 ? row : (y > root.contentHeight ? root.count - 1 : -1)
            }
        }

        onClicked: {
            selection.clear() // a plain click drops the multi-selection
            root.songClicked(modelData)
        }
        onCtrlClicked: selection.toggle(index)
        onRightClicked: selection.openMenuFor(index)
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 3

        policy: ScrollBar.AsNeeded

        lightMode: root.lightMode
    }
}
