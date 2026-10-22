import QtQuick
import QtQuick.Layouts

import Player.Browser
import Player.Listings
import Player.LocalLibrary
import Player.MediaTypes
import Player.MediaTransforms
import Player.PlayQueue
import Player.Primitives

ColumnLayout {
    id: root

    property real shortcutCoverWidth: 144
    property real shortcutCoverHeight: shortcutCoverWidth

    required property SearchBar searchBarItem

    property list<song> displayedTracks
    property real searchHighScore: -1

    Connections {
        target: LocalLibrary

        function onRefreshFinished() {
            root.displayedTracks = MediaFilter.search("", LocalLibrary).items;
            libraryChanges.enabled = true; // start reacting to dataChanged from now on
            target = null; // one-shot: stop listening after the first refresh completes
        }
    }

    Connections {
        id: libraryChanges
        target: LocalLibrary
        enabled: false // gated closed until the initial refresh finishes, above

        function onDataChanged() {
            if (root.searchBarItem.text === "") {
                root.displayedTracks = MediaFilter.search("", LocalLibrary).items;
            }
        }
    }


    SectionHeading {
        id: shortcutsHeading

        text: !root.searchBarItem.clearable ? qsTr("Your library") : qsTr("Search results")

        Layout.bottomMargin: root.shortcutCoverHeight / 16
    }

    Connections {
        target: root.searchBarItem

        function onTextChanged() {
            let response = MediaFilter.search(root.searchBarItem.text, LocalLibrary);
            root.displayedTracks =  response.items 
            root.searchHighScore =  response.highScore
        }
    }

    // soft threshold
    BigThinLabel {
        text: qsTr("The track you are looking for is likely not in your Library.")
        visible: root.searchHighScore > 0 && root.searchHighScore < 85
        topPadding: 5
        bottomPadding: 10
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