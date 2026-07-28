pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player
import Player.Primitives
import Player.StartPage

Item {
    id: root

    signal switchView() // to other specific view, currently leaving empty means "switch to fullscreen player"

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
            searchComponent: search
        }
    }

    ColumnLayout {
        id: rightCol

        anchors.top: parent.top
        anchors.left: leftCol.right
        anchors.right: parent.right
        anchors.bottom: nowplayingbar.stateModel.isMediaLoaded ? nowplayingbar.top : parent.bottom

        anchors.topMargin: 18 //  shortcutsList.shortcutCoverHeight / 8
        anchors.bottomMargin: nowplayingbar.stateModel.isMediaLoaded ? 18 : 0

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

            sourceComponent: folders
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

    Component {
        id: search

        Search {
            id: searchitem

            lateralAlignmentPadding: searchBar.leftPadding
            searchBarItem: searchBar
        }
    }

    MinibarPlayer {
        id: nowplayingbar

        anchors.left: leftCol.right
        anchors.right: fullscreenToggle.left
        anchors.bottom: parent.bottom

        topPadding: 20
        bottomPadding: topPadding

        visible: stateModel.isMediaLoaded

        leftPadding: searchBar.leftPadding
        rightPadding: leftPadding

        coverToMetadataSpacing: 8
    }

    LabeledButton {
        id: fullscreenToggle
        iconName: "window-maximize"

        text: "Fullscreen"

        opacity: (activeView.sourceComponent == home || nowplayingbar.visible || hovered) ? 1 : 0
        hoverEnabled: true

        padding: 20


        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.verticalCenter: nowplayingbar.verticalCenter

        onClicked: root.switchView()
    }

}