pragma ComponentBehavior: Bound
import QtQuick

import Player.Listings

GridView {
    id: root

    // model: the current folder model with members name, song
    property real songCoverWidth: 72
    property real songCoverHeight: songCoverWidth

    cellWidth: dummycard.width + (songCoverWidth / 12)
    cellHeight: dummycard.height + (songCoverWidth / 12)

    signal songClicked(var song);

    // selection

    property var selectedSources: []
    property var additionalMenuActions: []

    clip: true

    SongContextMenu {
        id: songContextMenu
        selectedUris: root.selectedSources

        onClearSelectionRequested: root.selectedSources = []

        actions: defaultActions.concat(root.additionalMenuActions)
    }

    delegate: Song {
        required property var model

        card: true

        title: model.title
        album: model.album
        artist: model.artist
        duration: model.duration
        cover: model.cover

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight

        // to visually mark it selected
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

    TallestDummyCard {
        id: dummycard
        visible: false

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight
    }
}