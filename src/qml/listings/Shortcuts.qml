pragma ComponentBehavior: Bound
import QtQuick

import Player.Listings
import Player.Primitives

ListView {
    id: root

    property real shortcutCoverWidth: 72
    property real shortcutCoverHeight: shortcutCoverWidth

    orientation: ListView.Horizontal

    spacing: shortcutCoverWidth / 12

    implicitHeight: contentItem.childrenRect.height

    signal songClicked (var song)

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
        id: shortcut

        card: true

        required property var model

        title: model.title
        artist: model.artist
        album: model.album
        duration: model.duration

        cover: model.cover

        coverWidth: root.shortcutCoverWidth
        coverHeight: root.shortcutCoverHeight

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
}