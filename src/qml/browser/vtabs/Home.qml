import QtQuick
import QtQuick.Layouts

import Player.Listings
import Player.LocalLibrary
import Player.PlayQueue
import Player.Primitives

ColumnLayout {
    id: root

    SectionHeading {
        id: shortcutsHeading

        text: qsTr("Shortcuts")

        Layout.bottomMargin: shortcutsList.shortcutCoverHeight / 16
    }

    Shortcuts {
        id: shortcutsList
        model: ShortcutsList

        Layout.fillWidth: true

        onSongClicked: (song) => {
            PlayQueue.switchTo(song.source)
        }
    }

    // absorb spacer
    Item {
        Layout.fillHeight: true 
    }
}
