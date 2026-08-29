import QtQuick
import QtQuick.Layouts

import Player.App
import Player.Browser
import Player.Fullscreen
import Player.Primitives

GridLayout {
    columns: 2
    columnSpacing: 20

    id: root

    property Window parentWindow // to inset window decorations

    signal switchView() // to other specific view, currently leaving empty means "switch to fullscreen player"

    // Empty space ftm, window handle
    Item {
        id: leftColHandle

        Layout.preferredWidth: leftCol.width
        Layout.preferredHeight: windexRow.height


        DragHandler {
            target: null
            
            onActiveChanged: {
                if (active) {
                    root.parentWindow.startSystemMove();
                }
            }
        }
    }

    // Search bar and window decorations for desktop
    RowLayout {
        id: windexRow

        Layout.fillWidth: true

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

            visible: searchBar.text !== ""

            filled: true

            magnify: false

            Layout.preferredHeight: searchBar.height
        }

        WindowDecorations {
            id: windex

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            window: root.parentWindow
        }
    }

    // Vertical tabs switch and window decorations for bigscreen
    ColumnLayout {
        id: leftCol

        Layout.preferredWidth: implicitWidth // create an alignment on the left limits of the elements
        Layout.maximumWidth: parent.width / 6 - searchBar.leftPadding

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

    // This page heading and browsing
    ColumnLayout {
        id: rightCol

        Layout.fillWidth: true
        Layout.fillHeight: true

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

        MinibarPlayer {
            id: nowplayingbar

            topPadding: 20
            bottomPadding: topPadding

            // this match may explain why this drifts below the window when it is too small
            Layout.preferredHeight: 48
            coverWidth: 48

            visible: stateModel.isMediaLoaded

            coverToMetadataSpacing: 8

            onMetadataClicked: root.switchView() // clicking cover and metadata also opens theater mode
        }

    }
}