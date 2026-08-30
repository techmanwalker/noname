pragma ComponentBehavior: Bound
import QtQuick

import Player.Listings
import Player.MediaTypes

GridView {
    id: root

    // model: the current folder model with members name, song
    property real songCoverWidth: 72
    property real songCoverHeight: songCoverWidth

    property real songLateralPadding: (songCoverWidth / 12)
    property real songVerticalPadding: (songCoverWidth / 12)

    cellWidth: dummycard.width + songLateralPadding
    cellHeight: dummycard.height + songVerticalPadding

    signal songClicked(song clicked_song);

    // selection

    property var selectedSources: []
    property var additionalMenuActions: []

    clip: true

    // Reduce frame drops while scrolling
    displayMarginBeginning: 1000
    displayMarginEnd: 1000

    SongContextMenu {
        id: songContextMenu
        selectedUris: root.selectedSources

        onClearSelectionRequested: root.selectedSources = []

        actions: defaultActions.concat(root.additionalMenuActions)
    }

    delegate: Item {
        id: delegateRoot

        required property song modelData

        width: GridView.view.cellWidth
        height: GridView.view.cellHeight

        Song {
            property song songItem: delegateRoot.modelData

            card: true

            anchors.fill: parent
            anchors.leftMargin: LayoutMirroring.enabled ? root.songLateralPadding : 0
            anchors.rightMargin: LayoutMirroring.enabled ? 0 : root.songLateralPadding
            anchors.bottomMargin: root.songVerticalPadding

            title: songItem.title
            metadata: songItem.printable_joint_metadata
            cover: songItem.cover
            duration: songItem.duration_mmss

            coverWidth: root.songCoverWidth
            coverHeight: root.songCoverHeight

            // to visually mark it selected
            selected: root.selectedSources.includes(songItem.source)

            onClicked: {
                // Standard click clears multi-selection
                root.selectedSources = []
                root.songClicked(songItem)
            }

            onCtrlClicked: {
                let sources = root.selectedSources
                let idx = sources.indexOf(songItem.source)
                
                if (idx === -1) {
                    sources.push(songItem.source)
                } else {
                    sources.splice(idx, 1)
                }
                
                // Reassign with slice() to guarantee the QML property binding is triggered
                root.selectedSources = sources.slice() 
            }

            onRightClicked: {
                // If right-clicked song isn't in current selection, select only it
                if (!root.selectedSources.includes(songItem.source)) {
                    root.selectedSources = [songItem.source]
                }
                songContextMenu.popup()
            }
        }
    }

    TallestDummyCard {
        id: dummycard
        visible: false

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight
    }
}