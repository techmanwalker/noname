import QtQuick
import QtQuick.Layouts

import Player.Browser
import Player.Theater
import Player.Listings
import Player.Presenter
import Player.PlayQueue

RowLayout {
    id: root

    required property SearchBar searchBar

    property alias activeVtab: activeView.currentIndex

    signal backButtonClicked_r () // notify, don't drive directly
    signal requestTheaterMode ()

    readonly property alias vtabsColWidth: vtabsCol.width

    // name -> stack index, generated from the TabPages in activeView: vtabs.<name>
    readonly property alias vtabs: vtabList.indices

    readonly property bool aPlaylistIsOpen:  activeVtab === vtabs.playlists && playlistsitem.aPlaylistIsOpen
    readonly property bool aDirectoryIsOpen: activeVtab === vtabs.folders   && foldersitem.aDirectoryIsOpen

    // tracks are being dragged out of the Tracks grid; Esc cancels that (see Main)
    readonly property bool isDraggingTracks: tracksitem.isDragging

    // tracks are being carried out of the Tracks grid, until their drop was delivered
    readonly property bool isCarryingTracks: tracksitem.isCarrying

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

            TabPage {
                name: "home"
                title: qsTr("Home")
                iconName: "user-home-symbolic"

                Home {
                    id: homeitem
                    anchors.fill: parent
                }
            }

            TabPage {
                name: "playlists"
                title: qsTr("Playlists")
                iconName: "view-media-playlist"

                Playlists {
                    id: playlistsitem
                    anchors.fill: parent
                }
            }

            TabPage {
                name: "folders"
                title: qsTr("Folders")
                iconName: "folder-symbolic"

                Folders {
                    id: foldersitem
                    anchors.fill: parent
                }
            }

            TabPage {
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
                    switch (root.activeVtab) {
                        case root.vtabs.playlists:
                            playlistsitem.backButtonClicked_r();
                            break;
                        case root.vtabs.folders: {
                            foldersitem.backButtonClicked_r();
                            break
                        }
                    }
                
                }
            }
        }

        // The only place that takes a drop of tracks: it owns its DropArea. Anywhere else
        // there is none, so the drop reports Qt.IgnoreAction and the ghosts play "rejected".
        MinibarPlayer {
            id: nowplayingbar

            topPadding: 20

            Layout.fillWidth: true
            Layout.rightMargin: 24 // scrollBarWidth (4) * 6

            // this match may explain why this drifts below the window when it is too small
            Layout.preferredHeight: implicitHeight
            coverWidth: 48

            stateModel: PlayerPresenter

            dropKeys: [tracksitem.dragKey]

            // Up while tracks are carried (so there is a target even with nothing loaded, and
            // it stays until the drop was delivered) and until a drop into an empty queue
            // has its tracks on it (batchAppend is asynchronous).
            visible: stateModel.isMediaLoaded
                || PlayQueue.count > 0
                || root.isCarryingTracks
                || nowplayingbar.isAcceptingADrop

            showDropHereBanner: root.isCarryingTracks

            coverToMetadataSpacing: 8
            volumeControlBottomPadding: 12

            onMetadataClicked: root.requestTheaterMode() // clicking cover and metadata also opens theater mode
        }

    }
}
