pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.MediaTypes
import Player.Primitives
import Player.PlayQueue

AutoscrollListView {
    id: root

    property bool lightMode: false

    property int playheadRowIdx

    // The playing row rests below the top edge by 0.4x its own height
    focusIndex: playheadRowIdx
    focusPosition: ListView.Beginning
    focusMarginRatio: 0.4
    autoscrollCooldownMs: 15000

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

    // selection

    property list<int> selectedIndices: []
    property alias additionalMenuActions: songContextMenu.extraActions

    SongContextMenu {
        id: songContextMenu
        selectedIndices: root.selectedIndices
        sourceModel: root.model

        onClearSelectionRequested: root.selectedIndices = []
        onAddSelectionToQueue: {
            // resolve tracks
            let tracks = [];

            root.selectedIndices.forEach((idx) => {
                tracks.push(root.model[idx].source)
            })

            PlayQueue.batchAppend(tracks)
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

        selected: root.selectedIndices.includes(index)

        onClicked: {
            // Standard click clears multi-selection
            root.selectedIndices = []
            root.songClicked(modelData)
        }

        onCtrlClicked: {
            let sources = root.selectedIndices
            let idx = sources.indexOf(index)
            
            if (idx === -1) {
                sources.push(index)
            } else {
                sources.splice(idx, 1)
            }
            
            // Reassign with slice() to guarantee the QML property binding is triggered
            root.selectedIndices = sources.slice() 
        }

        onRightClicked: {
            // If right-clicked song isn't in current selection, select only it
            if (!root.selectedIndices.includes(index)) {
                root.selectedIndices = [index]
            }
            songContextMenu.popup()
        }
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 3

        policy: ScrollBar.AsNeeded

        lightMode: root.lightMode
    }
}
