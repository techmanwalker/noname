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

    // The grabbed row, following the pointer. It paints as selected so it reads as lifted
    // above the rows it crosses.
    Loader {
        parent: root // viewport overlay, not scrolling content
        z: 20
        active: rowMove.moving
        enabled: false // never takes input

        // top-left in viewport coordinates; keeps the grab offset, so the row doesn't
        // jump to the pointer when the drag starts
        readonly property point topLeft: {
            const p = root.mapFromItem(null, mediaDrag.pointer.x, mediaDrag.pointer.y)
            return Qt.point(p.x - mediaDrag.grabOffset.x, p.y - mediaDrag.grabOffset.y)
        }
        x: topLeft.x
        y: topLeft.y

        sourceComponent: Song {
            readonly property song grabbed: mediaDrag.items[0]

            width: root.width - root.rightMargin
            height: 80

            selected: true
            lightMode: root.lightMode

            title:    grabbed.title
            cover:    grabbed.cover
            metadata: (grabbed.artist.length > 0 ? grabbed.artist : qsTr ("Unknown Artist"))
                + (grabbed.album.length > 0 ? " · " + grabbed.album : "")
            duration: grabbed.duration_mmss

            coverWidth:     root.songCoverWidth
            innerSpacing:   root.songInnerSpacing
            fadePadding:    root.songFadePadding
            lateralPadding: root.songLateralPadding
        }
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
