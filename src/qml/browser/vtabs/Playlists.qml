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
    }

    readonly property bool aPlaylistIsOpen: pl_stack.currentIndex === 1

    SectionHeading {
        id: shortcutsHeading

        text: pl_stack.currentIndex == 0 ? qsTr("Playlists") : root.selectedPlaylist.title;
    }

    MediaSelection {
        id: selection
        model: root.lists

        // sources = names in this case; must be an array, sourcesFor() iterates it
        sourcesOf: (row) => [PlaylistsList.itemAt(row).title]

        extraActions: [
            T.Action {
                text: qsTr("Delete playlist(s)", "in the tab that displays your Playlists", selection.indices.length)

                onTriggered: {
                    PlaylistsList.deletePlaylists(selection.sourcesFor(selection.indices))
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
            model: root.selectedPlaylist.songs

            onSongClicked: (clicked_song) => {
                // every tab manages click behavior on their own, Browser doesn't

                PlayQueue.respawnQueue(model)
                PlayQueue.playhead = PlayQueue.findBySource(clicked_song.source)
            }
        }
    }
}
