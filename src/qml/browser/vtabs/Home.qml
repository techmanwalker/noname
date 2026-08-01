import QtQuick
import QtQuick.Layouts

import Player.Listings
import Player.MediaSequences
import Player.Primitives

ColumnLayout {
    id: root

    property real shortcutCoverWidth: 144
    property real shortcutCoverHeight: shortcutCoverWidth

    // align with SearchBar text
    property real lateralAlignmentPadding: 0

    SectionHeading {
        id: shortcutsHeading

        text: qsTr("Shortcuts")

        Layout.leftMargin: root.lateralAlignmentPadding
        Layout.bottomMargin: root.shortcutCoverHeight / 16
    }

    Shortcuts {
        id: shortcutsList
        model: ShortcutsList

        Layout.fillWidth: true

        Layout.leftMargin: root.lateralAlignmentPadding

        shortcutCoverWidth: root.shortcutCoverWidth
        shortcutCoverHeight: root.shortcutCoverHeight

        clip: true // prevent sudden disappearing on the edges

        onSongClicked: (song) => {
            PlayQueue.switch_to(song.source)
        }
    }

    // absorb spacer
    Item {
        Layout.fillHeight: true 
    }
}