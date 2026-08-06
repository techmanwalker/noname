pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.App
import Player.Browser
import Player.Fullscreen


Item {
    id: root

    property Window parentWindow // to inset window decorations

    signal switchView() // to other specific view, currently leaving empty means "switch to fullscreen player"

    Item {
        id: leftCol

        width: parent.width / 6 - searchBar.leftPadding // create an alignment on the left limits of the elements
        
        anchors.top:    parent.top
        anchors.left:   parent.left
        anchors.bottom: parent.bottom

        ArrangementList {
            id: vtabs

            anchors.top: parent.top
            anchors.left: parent.left

            anchors.topMargin: searchBar.y + searchBar.height // todo: align with the search bar

            stack: activeView
            homeIndex: 0
            tracksIndex: 1
            foldersIndex: 2
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

        RowLayout {
            Layout.fillWidth: true

            SearchBar {
                id: searchBar

                Layout.preferredWidth: width
                Layout.preferredHeight: height

                Layout.alignment: Qt.AlignVCenter
            }

            WindowDecorations {
                id: windex

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter

                rightPadding: searchBar.leftPadding // keep symmetry

                window: root.parentWindow
            }
        }

        StackLayout {
            id: activeView

            Layout.fillWidth: true
            Layout.fillHeight: true

            currentIndex: vtabs.tracksIndex // Folders — matches the old sourceComponent: folders default

            Home {
                id: homeitem
                lateralAlignmentPadding: searchBar.leftPadding
            }

            Tracks {
                id: tracksitem
                lateralAlignmentPadding: searchBar.leftPadding
                searchBarItem: searchBar
            }

            Folders {
                id: foldersitem
                lateralAlignmentPadding: searchBar.leftPadding
            }
        }

    }

    MinibarPlayer {
        id: nowplayingbar

        anchors.left: leftCol.right
        anchors.bottom: parent.bottom

        topPadding: 20
        bottomPadding: topPadding

        visible: stateModel.isMediaLoaded

        leftPadding: searchBar.leftPadding
        rightPadding: leftPadding

        coverToMetadataSpacing: 8

        onFullscreenRequested: root.switchView()
    }

}