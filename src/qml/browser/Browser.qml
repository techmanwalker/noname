import QtQuick
import QtQuick.Layouts

import Player.Browser
import Player.Fullscreen
import Player.Presenter

RowLayout {
    id: root

    required property SearchBar searchBar

    property alias activeVtab: activeView.currentIndex

    signal requestTheaterMode ()

    readonly property alias vtabsColWidth: vtabsCol.width

    // Vertical tabs switch and window decorations for bigscreen
    ColumnLayout {
        id: vtabsCol
        
        Layout.minimumHeight: 0

        ArrangementList {
            id: vtabs

            Layout.alignment: Qt.AlignTop

            Layout.minimumHeight: 0

            stack: activeView
            homeIndex: 0
            tracksIndex: 1
            foldersIndex: 2
        }

        Item {
            Layout.fillHeight: true
        }

        Column {
            Layout.alignment: Qt.AlignBottom

            VTabButton {
                iconName: "view-fullscreen"
                text: qsTr("Theater mode")

                onClicked: root.requestTheaterMode()
            }
        }
    }

    // This page heading and browsing
    ColumnLayout {
        id: rightCol

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0

        StackLayout {
            id: activeView

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            clip: true

            currentIndex: vtabs.tracksIndex // Folders — matches the old sourceComponent: folders default

            Home {
                id: homeitem
            }

            Tracks {
                id: tracksitem
                searchBarItem: root.searchBar
            }

            Folders {
                id: foldersitem
            }
        }

        MinibarPlayer {
            id: nowplayingbar

            topPadding: 20

            // this match may explain why this drifts below the window when it is too small
            Layout.preferredHeight: implicitHeight
            coverWidth: 48

            stateModel: PlayerPresenter

            visible: stateModel.isMediaLoaded

            coverToMetadataSpacing: 8
            volumeControlBottomPadding: 12

            onMetadataClicked: root.requestTheaterMode() // clicking cover and metadata also opens theater mode
        }

    }
}