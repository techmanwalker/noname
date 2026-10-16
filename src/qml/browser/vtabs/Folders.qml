import QtQuick
import QtQuick.Layouts

import Player.Browser
import Player.Listings
import Player.Primitives
import Player.MediaTypes
import Player.LocalLibrary
import Player.PlayQueue

ColumnLayout {
    id: root

    /*  when a directory under the FolderList is clicked, its .songs list
        will be copied here to display its songs */
    property directory activeDirectoryModel

    SectionHeading {
        id: shortcutsHeading

        text: qsTr("Folders");

        Layout.bottomMargin: currentdirectory.songCoverHeight / 16
    }

    FolderList {
        Layout.bottomMargin: 18 // songCoverHeight / 8

        model: LocalLibrary

        Layout.fillWidth: true
        Layout.preferredHeight: dummyfolder.height

        orientation: ListView.Horizontal

        onDirectorySwitched: (dir) => {
            root.activeDirectoryModel = dir
        }
    }

    SongGrid {
        id: currentdirectory
        model: root.activeDirectoryModel.songs

        Layout.fillWidth: true
        Layout.fillHeight: true

        songCoverWidth: 144

        reuseItems: true

        onSongClicked: (song) => {
            PlayQueue.switchTo(song.source)
        }
    }

    Folder {
        id: dummyfolder

        text: "L"

        visible: false
    }
}