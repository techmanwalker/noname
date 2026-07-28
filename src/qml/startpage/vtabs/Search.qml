import QtQuick
import QtQuick.Layouts

import Player.Primitives
import Player.MediaSequences
import Player.StartPage

ColumnLayout {
    id: root

    property real shortcutCoverWidth: 144
    property real shortcutCoverHeight: shortcutCoverWidth

    // align with SearchBar text
    property real lateralAlignmentPadding: 0

    required property SearchBar searchBarItem

    SectionHeading {
        id: shortcutsHeading

        text: "Search in you library"

        Layout.leftMargin: root.lateralAlignmentPadding
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