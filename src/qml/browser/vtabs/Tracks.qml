import QtQuick
import QtQuick.Layouts

import Player.Browser
import Player.Listings
import Player.LocalLibrary
import Player.MediaTypes
import Player.PlayQueue
import Player.Primitives

ColumnLayout {
    id: root

    property real shortcutCoverWidth: 144
    property real shortcutCoverHeight: shortcutCoverWidth

    required property SearchBar searchBarItem

    property list<song> displayedTracks

    Connections {
        target: LocalLibrary // qmllint disable

        function onRefreshFinished() {
            root.displayedTracks = MediaFilter.search("", LocalLibrary);
            libraryChanges.enabled = true; // start reacting to dataChanged from now on
            target = null; // one-shot: stop listening after the first refresh completes
        }
    }

    Connections {
        id: libraryChanges
        target: LocalLibrary // qmllint disable
        enabled: false // gated closed until the initial refresh finishes, above

        function onDataChanged() {
            if (root.searchBarItem.text === "") {
                root.displayedTracks = MediaFilter.search("", LocalLibrary);
            }
        }
    }


    SectionHeading {
        id: shortcutsHeading

        text: qsTr("Your library")

        Layout.bottomMargin: root.shortcutCoverHeight / 16
    }

    Connections {
        target: root.searchBarItem

        function onTextChanged() {
            
            root.displayedTracks = MediaFilter.search(root.searchBarItem.text, LocalLibrary);
        }
    }

    SongGrid {
        model: root.displayedTracks

        Layout.fillWidth: true
        Layout.fillHeight: true

        songCoverWidth: 144
        scrollBarWidth: 4

        reuseItems: true

        onSongClicked: function(clicked_song: song) {
            PlayQueue.switchTo(clicked_song.source)
        }

        additionalMenuActions: [
            {
                text: qsTr("Play all"),
                action: () => PlayQueue.respawnQueue(LocalLibrary.flattenedSources())
            }
        ]
    }
}