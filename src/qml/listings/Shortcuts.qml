pragma ComponentBehavior: Bound
import QtQuick

import Player.Listings
import Player.MediaTypes
import Player.PlayQueue

ListView {
    id: root

    property real shortcutCoverWidth: 72
    property real shortcutCoverHeight: shortcutCoverWidth

    orientation: ListView.Horizontal

    spacing: shortcutCoverWidth / 12

    implicitHeight: contentItem.childrenRect.height

    clip: true

    signal songClicked (song clicked_song)

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
        id: shortcut

        card: true

        required property song model
        required property int  index

        title: model.title
        metadata: (model.artist.length > 0 ? model.artist : qsTr ("Unknown Artist"))
            + (model.album.length > 0 ? " · " + model.album : "")
        duration: model.duration_mmss

        cover: model.cover

        coverWidth: root.shortcutCoverWidth
        coverHeight: root.shortcutCoverHeight

        // to visually mark it selected
        selected: root.selectedIndices.includes(index)

        onClicked: {
            // Standard click clears multi-selection
            root.selectedIndices = []
            root.songClicked(model)
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
}
