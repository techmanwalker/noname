import QtQuick
import QtQuick.Layouts

import Player.Browser
import Player.Fullscreen
import Player.Presenter

RowLayout {
    id: root

    required property SearchBar searchBar

    property alias activeVtab: activeView.currentIndex

    signal backButtonClicked_r () // notify, don't drive directly
    signal requestTheaterMode ()

    readonly property alias vtabsColWidth: vtabsCol.width

    // name -> stack index, generated from the VTabPages in activeView: vtabs.<name>
    readonly property alias vtabs: vtabList.indices

    readonly property bool aPlaylistIsOpen: activeVtab === vtabs.playlists && playlistsitem.aPlaylistIsOpen

    // Vertical tabs switch and window decorations for bigscreen
    ColumnLayout {
        id: vtabsCol
        
        Layout.minimumHeight: 0

        ArrangementList {
            id: vtabList

            Layout.alignment: Qt.AlignTop

            Layout.minimumHeight: 0

            stack: activeView
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

            Component.onCompleted: currentIndex = root.vtabs.tracks

            VTabPage {
                name: "home"
                title: qsTr("Home")
                iconName: "user-home-symbolic"

                Home {
                    id: homeitem
                    anchors.fill: parent
                }
            }

            VTabPage {
                name: "playlists"
                title: qsTr("Playlists")
                iconName: "view-media-playlist"

                Playlists {
                    id: playlistsitem
                    anchors.fill: parent
                }
            }

            VTabPage {
                name: "folders"
                title: qsTr("Folders")
                iconName: "folder-symbolic"

                Folders {
                    id: foldersitem
                    anchors.fill: parent
                }
            }

            VTabPage {
                name: "tracks"
                title: qsTr("Tracks")
                iconName: "library-music-symbolic"

                Tracks {
                    id: tracksitem
                    anchors.fill: parent
                    searchBarItem: root.searchBar
                }
            }

            Connections {
                target: root
                function onBackButtonClicked_r () {
                    playlistsitem.backButtonClicked_r()
                }
            }
        }

        MinibarPlayer {
            id: nowplayingbar

            topPadding: 20

            Layout.rightMargin: 24 // scrollBarWidth (4) * 6

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