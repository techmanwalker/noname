pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.Browser

Column {
    id: root

    property StackLayout stack

    required property int homeIndex
    required property int tracksIndex
    required property int foldersIndex

    VTabButton {
        iconName: "user-home-symbolic"
        text: qsTr("Home")

        onClicked: if (root.stack) root.stack.currentIndex = root.homeIndex
    }

    VTabButton {
        iconName: "media-album-cover"
        text: qsTr("Albums")

        visible: false // still unsupported
    }

    VTabButton {
        iconName: "view-media-artist"
        text: qsTr("Artists")

        visible: false // still unsupported
    }

    VTabButton {
        iconName: "view-media-playlist"
        text: qsTr("Playlists")

        visible: false // still unsupported
    }

    VTabButton {
        iconName: "folder-symbolic"
        text: qsTr("Folders")

        onClicked: if (root.stack) root.stack.currentIndex = root.foldersIndex
    }

    VTabButton {
        iconName: "library-music-symbolic"
        text: qsTr("Tracks")

        onClicked: if (root.stack) root.stack.currentIndex = root.tracksIndex
    }

    // AllTracks was delegated to the Search page, an empty search query returns your entire library
}