pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

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

            MediaSelection {
                id: selection
                model: root.lists

                // a playlist stands for all of its tracks
                sourcesOf: (row) => {
                    const out = []
                    root.lists[row].songs.forEach(s => out.push(s.source))
                    return out
                }
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
