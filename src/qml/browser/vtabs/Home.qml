import QtQuick
import QtQuick.Layouts

import Player.Listings
import Player.LocalLibrary
import Player.PlayQueue
import Player.Primitives

ColumnLayout {
    id: root

    property real shortcutCoverWidth: 144
    property real shortcutCoverHeight: shortcutCoverWidth

    SectionHeading {
        id: shortcutsHeading

        text: qsTr("Shortcuts")

        Layout.bottomMargin: root.shortcutCoverHeight / 16
    }

    Shortcuts {
        id: shortcutsList
        model: ShortcutsList

        Layout.fillWidth: true

        shortcutCoverWidth: root.shortcutCoverWidth
        shortcutCoverHeight: root.shortcutCoverHeight

        onSongClicked: (song) => {
            PlayQueue.switch_to(song.source)
        }
    }

    // absorb spacer
    Item {
        Layout.fillHeight: true 
    }
}