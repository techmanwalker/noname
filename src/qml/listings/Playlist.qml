pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.Primitives
import Player.PlayQueue

ListView {
    id: root

    property int songCoverWidth:  48
    property int songCoverHeight: songCoverWidth
    property int scrollBarWidth: 4

    // expose to FullscreenPlayer
    property real songInnerSpacing: 8
    property real songFadePadding: 10
    property real songLateralPadding: 15

    // Right padding reserves space for the scrollbar so it appears
    // to float outside the list content without overlapping it
    rightMargin: scrollbar.logicalWidth

    signal songClicked(var song)

    // selection

    property var selectedSources: []
    property var additionalMenuActions: []

    SongContextMenu {
        id: songContextMenu
        selectedUris: root.selectedSources

        onClearSelectionRequested: root.selectedSources = []

        actions: defaultActions.concat(root.additionalMenuActions)
    }

    delegate: Song {
        id: song

        required property var model
        
        width: root.width - root.rightMargin

        height: 80

        title:      model.title
        cover:      model.cover
        metadata:     model.printable_joint_metadata
        duration:   model.duration_mmss

        coverWidth: root.songCoverWidth

        innerSpacing:   root.songInnerSpacing
        fadePadding:    root.songFadePadding
        lateralPadding: root.songLateralPadding

        playing: PlayQueue.playhead === PlayQueue.index(model.index, 0) // qmllint disable

        showSeparator: true

        clip: true

        selected: root.selectedSources.includes(model.source)

        onClicked: {
            // Standard click clears multi-selection
            root.selectedSources = []
            root.songClicked(model)
        }

        onCtrlClicked: {
            let sources = root.selectedSources
            let idx = sources.indexOf(model.source)
            
            if (idx === -1) {
                sources.push(model.source)
            } else {
                sources.splice(idx, 1)
            }
            
            // Reassign with slice() to guarantee the QML property binding is triggered
            root.selectedSources = sources.slice() 
        }

        onRightClicked: {
            // If right-clicked song isn't in current selection, select only it
            if (!root.selectedSources.includes(model.source)) {
                root.selectedSources = [model.source]
            }
            songContextMenu.popup()
        }
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 3

        policy: ScrollBar.AsNeeded
    }
}