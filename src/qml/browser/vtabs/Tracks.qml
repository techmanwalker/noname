import QtQuick
import QtQuick.Layouts

import Player.Browser
import Player.Listings
import Player.MediaSequences
import Player.Primitives

ColumnLayout {
    id: root

    property real shortcutCoverWidth: 144
    property real shortcutCoverHeight: shortcutCoverWidth

    // align with SearchBar text
    property real lateralAlignmentPadding: 0

    required property SearchBar searchBarItem

    Connections {
        target: LocalLibrary

        function onRefreshFinished() {
            SearchResults.performSearch("", LocalLibrary);
            libraryChanges.enabled = true; // start reacting to dataChanged only from here on
            target = null; // one-shot: stop listening after the first refresh completes
        }
    }

    Connections {
        id: libraryChanges
        target: LocalLibrary
        enabled: false // gated closed until the initial refresh finishes, above

        function onDataChanged() {
            if (root.searchBarItem.text == "") {
                SearchResults.performSearch("", LocalLibrary);
            }
        }
    }


    SectionHeading {
        id: shortcutsHeading

        text: qsTr("Your library")

        Layout.leftMargin: root.lateralAlignmentPadding
        Layout.bottomMargin: root.shortcutCoverHeight / 16
    }

    Connections {
        target: root.searchBarItem

        function onTextChanged() {
            
            SearchResults.performSearch(root.searchBarItem.text, LocalLibrary);
        }
    }

    SongGrid {
        model: SearchResults

        Layout.fillWidth: true
        Layout.fillHeight: true

        Layout.leftMargin: root.lateralAlignmentPadding

        songCoverWidth: 144

        clip: true

        reuseItems: true

        onSongClicked: (song) => {
            PlayQueue.switch_to(song.source)
        }
    }
}