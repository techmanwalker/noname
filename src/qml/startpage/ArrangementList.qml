pragma ComponentBehavior: Bound
import QtQuick

import Player.StartPage

Column {
    id: root

    property Loader loader
    
    // needs to be injected from parent
    property real vtabPadding: 24
    property real vtabLeftPadding: vtabPadding
    property real vtabRightPadding: vtabPadding
    property real vtabTopPadding: vtabPadding
    property real vtabBottomPadding: vtabBottomPadding

    property real vtabSpacing: vtabPadding / 2

    required property Component homeComponent
    required property Component foldersComponent

    VTab {
        iconName: "user-home-symbolic"
        text: "Home"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        onClicked: if (root.loader) root.loader.sourceComponent = root.homeComponent
    }

    VTab {
        iconName: "media-album-cover"
        text: "Albums"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        visible: false // still unsupported
    }

    VTab {
        iconName: "view-media-artist"
        text: "Artists"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        visible: false // still unsupported
    }

    VTab {
        iconName: "view-media-playlist"
        text: "Playlists"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        visible: false // still unsupported
    }

    VTab {
        iconName: "folder-symbolic"
        text: "Folders"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        onClicked: if (root.loader) root.loader.sourceComponent = root.foldersComponent
    }

    VTab {
        iconName: "view-media-track"
        text: "All tracks"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        visible: false // still unsupported
    }
}