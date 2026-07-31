import QtQuick
import QtQuick.Layouts

import Player.Browser
import Player.Primitives
import Player.MediaSequences

ColumnLayout {
    id: root

    // align with SearchBar text
    property real lateralAlignmentPadding: 0

    /*  when a directory under the FolderList is clicked, its .songs list
        will be copied here to display its songs */
    property var activeDirectoryModel

    Component.onCompleted: {
        if (LocalLibrary.count > 0) {
            const songs = LocalLibrary.readRole(0, "songs")
            if (songs && songs.length !== 0) root.activeDirectoryModel = songs
        }
    }

    SectionHeading {
        id: shortcutsHeading

        text: "Folders"

        Layout.leftMargin: root.lateralAlignmentPadding
        Layout.bottomMargin: currentdirectory.songCoverHeight / 16
    }

    FolderList {
        Layout.leftMargin: root.lateralAlignmentPadding - dummyfolder.leftPadding
        Layout.bottomMargin: 18 // songCoverHeight / 8

        model: LocalLibrary

        Layout.fillWidth: true
        Layout.preferredHeight: dummyfolder.height

        orientation: ListView.Horizontal

        onDirectorySwitched: (songs) => {
            if (songs.length != 0) {
                root.activeDirectoryModel = songs;
            }
        }

        clip: true
    }

    SongGrid {
        id: currentdirectory
        model: root.activeDirectoryModel

        Layout.fillWidth: true
        Layout.fillHeight: true

        songCoverWidth: 144

        clip: true

        reuseItems: true

        onSongClicked: (song) => {
            PlayQueue.switch_to(song.source)
        }
    }

    Folder {
        id: dummyfolder

        text: "L"

        visible: false
    }
}