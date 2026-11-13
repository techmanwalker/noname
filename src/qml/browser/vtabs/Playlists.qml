pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Player.Primitives
import Player.PlayQueue
import Player.Listings
import Player.LocalLibrary
import Player.MediaTypes

ColumnLayout {
    id: root
    property var lists: PlaylistsList

    // expose to Theater
    property real songInnerSpacing: 8
    property real songFadePadding: 10
    property real songLateralPadding: 15

    // to display the tracklist, title and more
    property playlist selectedPlaylist

    signal backButtonClicked_r () // from top to down in the hierarchy, marked
    signal songClicked (song clicked_song)

    onBackButtonClicked_r: {
        pl_stack.currentIndex = 0
        openPlaylist.selectedIndices = []
    }

    readonly property bool aPlaylistIsOpen: pl_stack.currentIndex === 1

    // Tags this tab's drags (playlists, or tracks of the open one) for the minibar
    readonly property string dragKey: "noname/playlists"
    readonly property int maxDraggedRows: 5

    readonly property bool isDragging: selectionDrag.armed  || openPlaylist.isDragging
    readonly property bool isCarrying: selectionDrag.active || openPlaylist.isCarrying
    SectionHeading {
        id: shortcutsHeading

        text: pl_stack.currentIndex == 0 ? qsTr("Playlists") : root.selectedPlaylist.title;
    }

    MediaSelection {
        id: selection
        model: root.lists

        // a row is a playlist; its sources are those of its tracks
        songAt:    (row) => PlaylistsList.itemAt(row)
        sourcesOf: (row) => PlaylistsList.itemAt(row).songs.map(s => s.source)

        extraActions: [
            T.Action {
                text: qsTr("Delete playlist(s)", "in the tab that displays your Playlists", selection.indices.length)

                onTriggered: {
                    PlaylistsList.deletePlaylists(selection.indices.map(row => PlaylistsList.itemAt(row).title))

                    // the model resets after the refresh, so the selected rows no longer mean anything
                    selection.clear()
                }
            }
        ]
    }
    
    StackLayout {
        id: pl_stack
        Layout.fillWidth: true
        Layout.fillHeight: true

        // playlist of playlists
        ListView {
            id: pl

            model: root.lists

                        Layout.fillWidth: true
            Layout.fillHeight: true

            SelectionDrag {
                id: selectionDrag
                view: pl
                selection: selection
                maxSelectedItems: root.maxDraggedRows
                cancelSequences: [StandardKey.Cancel]
            }

            GhostSpawner {
                selectionDrag: selectionDrag
                cards: false
                dragKey: root.dragKey
                rowWidth: pl.width

                metadataOf: (playlist) => qsTr("%n track(s)", "number of tracks in this playlist", playlist.songs.length)
            }

            delegate: MediaDelegate {
                id: song

                required property playlist model
                required property int index

                width: pl.width

                height: 80

                title:      model.title
                cover:      model.cover
                metadata:   qsTr("%n track(s)", "number of tracks in this playlist", model.songs.length)
                duration:   model.duration_mmss

                innerSpacing:   root.songInnerSpacing
                fadePadding:    root.songFadePadding
                lateralPadding: root.songLateralPadding

                showSeparator: true

                clip: true

                selected: selection.isSelected(index)

                opacity: selectionDrag.carries(index) ? 0.4 : 1

                SelectionDrag.Arm   { controller: selectionDrag; row: song.index }
                SelectionDrag.Track { controller: selectionDrag; row: song.index }

                onClicked: {
                    selection.clear()

                    // lacking the source file still
                    root.selectedPlaylist = model

                    // switch the stack to single playlist view
                    pl_stack.currentIndex = 1
                }
                onCtrlClicked: selection.toggle(index)
                onRightClicked: selection.openMenuFor(index)
            }
        }

        Playlist {
            id: openPlaylist

            model: root.selectedPlaylist.songs

            maxDraggedSongs: root.maxDraggedRows
            dragKey: root.dragKey

            onSongClicked: (clicked_song) => {
                // every tab manages click behavior on their own, Browser doesn't

                PlayQueue.respawnQueue(model)
                PlayQueue.playhead = PlayQueue.findBySource(clicked_song.source)
            }
        }
    }
}
