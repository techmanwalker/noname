pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.Primitives

Column {
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
    required property int searchIndex
    required property int foldersIndex

    LabeledButton {
        iconName: "user-home-symbolic"
        text: "Home"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        onClicked: if (root.stack) root.stack.currentIndex = root.homeIndex
    }

    LabeledButton {
        iconName: "search"
        text: "Search"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        onClicked: if (root.stack) root.stack.currentIndex = root.searchIndex
    }

    LabeledButton {
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

    LabeledButton {
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

    LabeledButton {
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

    LabeledButton {
        iconName: "folder-symbolic"
        text: "Folders"

        padding: root.vtabPadding
        leftPadding: root.vtabLeftPadding
        rightPadding: root.vtabRightPadding
        topPadding: root.vtabTopPadding
        bottomPadding: root.vtabBottomPadding

        spacing: root.vtabSpacing

        onClicked: if (root.stack) root.stack.currentIndex = root.foldersIndex
    }

    LabeledButton {
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