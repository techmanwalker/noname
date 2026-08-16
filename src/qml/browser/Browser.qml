pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.App
import Player.Browser
import Player.Fullscreen
import Player.Primitives

Item {
    id: root

    property Window parentWindow // to inset window decorations

    signal switchView() // to other specific view, currently leaving empty means "switch to fullscreen player"

    ColumnLayout {
        id: leftCol

        width: Math.max(implicitWidth, parent.width / 6 - searchBar.leftPadding) // create an alignment on the left limits of the elements
        
        anchors.top:    parent.top
        anchors.left:   parent.left
        anchors.bottom: parent.bottom

        Item {
            id: leftColHandle
            Layout.fillWidth: true
            Layout.preferredHeight: searchBar.y + searchBar.height // todo: align with the search bar

            DragHandler {
                target: null
                
                onActiveChanged: {
                    if (active) {
                        root.parentWindow.startSystemMove();
                    }
                }
            }
        }

        ArrangementList {
            id: vtabs

            Layout.alignment: Qt.AlignTop

            stack: activeView
            homeIndex: 0
            tracksIndex: 1
            foldersIndex: 2
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            DragHandler {
                target: null
                
                onActiveChanged: {
                    if (active) {
                        root.parentWindow.startSystemMove();
                    }
                }
            }
        }

        Column {
            Layout.alignment: Qt.AlignBottom

            Layout.bottomMargin: leftColHandle.height

            VTabButton {
                iconName: "view-fullscreen"
                text: qsTr("Theater mode")

                onClicked: root.switchView()
            }
        }
    }

    ColumnLayout {
        id: rightCol

        anchors.top: parent.top
        anchors.left: leftCol.right
        anchors.right: parent.right
        anchors.bottom: nowplayingbar.stateModel.isMediaLoaded ? nowplayingbar.top : parent.bottom

        anchors.topMargin: 20 //  shortcutsList.shortcutCoverHeight / 8
        anchors.bottomMargin: nowplayingbar.stateModel.isMediaLoaded ? 18 : 0

        RowLayout {
            Layout.fillWidth: true

            Layout.rightMargin:Math.max(20 // VTabButton lateral padding
                - windex.closeButtonRightPadding // inset in VTabButton padding
                - 3 // the close icon has 6 pixels of empty space on its 24 px version

                , 0 /* safety*/)

            DragHandler {
                target: null
                
                onActiveChanged: {
                    if (active) {
                        root.parentWindow.startSystemMove();
                    }
                }
            }

            SearchBar {
                id: searchBar

                Layout.preferredWidth: width
                Layout.preferredHeight: height

                Layout.alignment: Qt.AlignVCenter

                onTextEdited: activeView.currentIndex = vtabs.tracksIndex
            }

            ResizableButton {
                text: qsTr("Clear")
                onClicked: {
                    searchBar.text = ""
                    searchBar.textEdited()
                }

                leftPadding: searchBar.leftPadding / 2
                rightPadding: searchBar.rightPadding / 2

                Layout.preferredHeight: searchBar.height
            }

            WindowDecorations {
                id: windex

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter

                window: root.parentWindow
            }
        }

        StackLayout {
            id: activeView

            Layout.fillWidth: true
            Layout.fillHeight: true

            currentIndex: vtabs.tracksIndex // Folders — matches the old sourceComponent: folders default

            Layout.rightMargin: 20 // should be consistent with VTabButton lateral padding

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

        width: Math.min(rightCol.width - 20, implicitWidth)
        height: 96

        visible: stateModel.isMediaLoaded

        coverToMetadataSpacing: 8

        onMetadataClicked: root.switchView() // clicking cover and metadata also opens theater mode
    }

}