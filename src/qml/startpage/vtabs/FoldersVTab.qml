import QtQuick
import QtQuick.Layouts

import Player
import Player.Primitives
import Player.MediaSequences
import Player.StartPage

ColumnLayout {
    id: root

    // align with SearchBar text
    property real lateralAlignmentPadding: 0

    /*  when a directory under the FolderList is clicked, its .songs list
        will be copied here to display its songs */
    property var activeDirectoryModel

    SectionHeading {
        id: shortcutsHeading

        text: "Folders"

        Layout.leftMargin: root.lateralAlignmentPadding
    }

    FolderList {
        model: LocalLibrary

        Layout.fillWidth: true
        Layout.preferredHeight: 20

        onDirectorySwitched: (songs) => {
            if (songs.length != 0) {
                root.activeDirectoryModel = songs;
            }
        }

        clip: true
    }

    Playlist {
        model: root.activeDirectoryModel

        reuseItems: true // tons of songs moving

        Layout.fillWidth: true
        Layout.fillHeight: true

        clip: true
    }
}