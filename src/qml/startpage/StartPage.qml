import QtQuick

import Player.MediaSequences
import Player.Primitives
import Player.StartPage

Item {
    id: root

    Item {
        id: leftCol

        width: parent.width / 4 - searchBar.leftPadding // create an alignment on the left limits of the elements
        
        anchors.top:    parent.top
        anchors.left:   parent.left
        anchors.bottom: parent.bottom

        ArrangementList {
            anchors.top: parent.top
            anchors.left: parent.left

            anchors.topMargin: searchBar.y + searchBar.height // align with the search bar
            anchors.leftMargin: shortcutsList.spacing

            vtabLeftPadding: shortcutsList.spacing * 2
            vtabRightPadding: vtabLeftPadding
            vtabTopPadding: Window.height / shortcutsList.spacing / 8 // link to window height, items are closer when window is shorter
            vtabBottomPadding: vtabTopPadding
        }
    }

    Item {
        id: rightCol

        anchors.top: parent.top
        anchors.left: leftCol.right
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        SearchBar {
            id: searchBar

            anchors.top: parent.top
            anchors.left: parent.left

            anchors.topMargin: shortcutsList.shortcutCoverHeight / 6 // compensate the "Shortcuts" top padding
        }

        SectionHeading {
            id: shortcutsHeading

            text: "Shortcuts"

            anchors.top: searchBar.bottom
            anchors.left: parent.left

            anchors.topMargin: shortcutsList.shortcutCoverHeight / 8
            anchors.leftMargin: searchBar.leftPadding
        }

        Shortcuts {
            id: shortcutsList
            model: ShortcutsList

            anchors.top: shortcutsHeading.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            anchors.topMargin: shortcutsList.shortcutCoverHeight / 16
            anchors.leftMargin: searchBar.leftPadding

            shortcutCoverWidth: 144

            clip: true // prevent sudden disappearing on the edges
        }
    }
}