pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.Primitives

ColumnLayout {
    id: root

    property StackLayout stack
    
    // needs to be injected from parent
    property real vtabPadding: 24
    property real vtabLeftPadding: vtabPadding
    property real vtabRightPadding: vtabPadding
    property real vtabTopPadding: vtabPadding
    property real vtabBottomPadding: vtabBottomPadding

    property real vtabSpacing: vtabPadding / 2

    required property int homeIndex
    required property int tracksIndex
    required property int foldersIndex

    ResizableButton {
        iconName: "user-home-symbolic"
        text: qsTr("Home")

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        magnify: true

        spacing: root.vtabSpacing

        onClicked: if (root.stack) root.stack.currentIndex = root.homeIndex
    }

    ResizableButton {
        iconName: "media-album-cover"
        text: qsTr("Albums")

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        visible: false // still unsupported
    }

    ResizableButton {
        iconName: "view-media-artist"
        text: qsTr("Artists")

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        visible: false // still unsupported
    }

    ResizableButton {
        iconName: "view-media-playlist"
        text: qsTr("Playlists")

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        visible: false // still unsupported
    }

    ResizableButton {
        iconName: "folder-symbolic"
        text: qsTr("Folders")

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        magnify: true

        onClicked: if (root.stack) root.stack.currentIndex = root.foldersIndex
    }

    ResizableButton {
        iconName: "library-music-symbolic"
        text: qsTr("Tracks")

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        magnify: true

        onClicked: if (root.stack) root.stack.currentIndex = root.tracksIndex
    }

    // AllTracks was delegated to the Search page, an empty search query returns your entire library
}