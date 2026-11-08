pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.MediaTypes
import Player.Primitives
import Player.PlayQueue

ListView {
    id: root

    property bool lightMode: false

    property int playheadRowIdx

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

    property list<url> selectedSources: []
    property alias additionalMenuActions: songContextMenu.extraActions

    onPlayheadRowIdxChanged: {
        if (playheadRowIdx < 0)
            return

        const targetItem = itemAtIndex(playheadRowIdx)
        if (!targetItem)
            return

        const itemY = targetItem.y
        const itemHeight = targetItem.height
        const itemBottom = itemY + itemHeight

        // Check if the item goes offscreen or does not fit entirely in the viewport
        if (itemY < root.contentY || itemBottom > root.contentY + root.height) {
            const targetY = itemY - 0.4 * targetItem.height
            const maxContentY = Math.max(0, root.contentHeight - root.height)
            root.contentY = Math.min(Math.max(0, targetY), maxContentY)
        }
    }

    SongContextMenu {
        id: songContextMenu
        selectedUris: root.selectedSources

        onClearSelectionRequested: root.selectedSources = []
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

        selected: root.selectedSources.includes(modelData.source)

        onClicked: {
            // Standard click clears multi-selection
            root.selectedSources = []
            root.songClicked(modelData)
        }

        onCtrlClicked: {
            let sources = root.selectedSources
            let idx = sources.indexOf(modelData.source)
            
            if (idx === -1) {
                sources.push(modelData.source)
            } else {
                sources.splice(idx, 1)
            }
            
            // Reassign with slice() to guarantee the QML property binding is triggered
            root.selectedSources = sources.slice() 
        }

        onRightClicked: {
            // If right-clicked song isn't in current selection, select only it
            if (!root.selectedSources.includes(modelData.source)) {
                root.selectedSources = [modelData.source]
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