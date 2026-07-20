pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player
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
            id: vtabs

            anchors.top: parent.top
            anchors.left: parent.left

            anchors.topMargin: searchBar.y + searchBar.height // todo: align with the search bar

            vtabLeftPadding: 20
            vtabRightPadding: vtabLeftPadding
            vtabTopPadding: 12 // link to window height, items are closer when window is shorter
            vtabBottomPadding: vtabTopPadding

            loader: activeView
            homeComponent: home
            foldersComponent: folders
        }
    }

    ColumnLayout {
        id: rightCol

        anchors.top: parent.top
        anchors.left: leftCol.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom // later nowplayingbar.top

        Layout.topMargin: 18 //  shortcutsList.shortcutCoverHeight / 8

        SearchBar {
            id: searchBar

            Layout.topMargin: 24 // songCoverWidth / 6

            Layout.preferredWidth: width
            Layout.preferredHeight: height
        }

        Loader {
            id: activeView

            Layout.fillWidth: true
            Layout.fillHeight: true

            sourceComponent: home
        }
    }

    Component {
        id: home

        Home {
            id: homeitem

            lateralAlignmentPadding: searchBar.leftPadding
        }
    }

    Component {
        id: folders

        Folders {

            id: foldersitem

            lateralAlignmentPadding: searchBar.leftPadding
        }
    }

    Item {
        id: nowplayingbar

        anchors.left: leftCol.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: minibarplayer.height

        visible: false

        MinibarPlayer {
            id: minibarplayer

            anchors.fill: parent

            topPadding: 20
            bottomPadding: topPadding

            leftPadding: searchBar.leftPadding
            rightPadding: leftPadding

            spacing: 24

            coverToMetadataSpacing: 8
        }
    }
}