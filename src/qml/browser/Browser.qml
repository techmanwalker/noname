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

    // This page heading and browsing.
    // Wraps the column, like Theater's right column, so the minibar's layout is untouched:
    // the whole column hears the drag, the minibar is the only place that takes the drop.
    DropArea {
        id: rightCol_d

        keys: [tracksitem.dragKey]

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0

        onDropped: (drop) => {
            const dragged = drop.source as SelectionDrag
            if (!dragged || !nowplayingbar.visible) {
                drop.accepted = false
                return
            }

            const p = nowplayingbar.mapFromItem(rightCol_d, drop.x, drop.y)
            if (!nowplayingbar.contains(p)) {
                drop.accepted = false
                return
            }

            PlayQueue.batchAppend(dragged.sources)
            drop.acceptProposedAction()
        }

        ColumnLayout {
            id: rightCol

            anchors.fill: parent

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

            MinibarPlayer {
                id: nowplayingbar

                topPadding: 20

                Layout.rightMargin: 24 // scrollBarWidth (4) * 6

                // this match may explain why this drifts below the window when it is too small
                Layout.preferredHeight: implicitHeight
                coverWidth: 48

                stateModel: PlayerPresenter

                visible: stateModel.isMediaLoaded || PlayQueue.count > 0

                coverToMetadataSpacing: 8
                volumeControlBottomPadding: 12

                onMetadataClicked: root.requestTheaterMode() // clicking cover and metadata also opens theater mode
            }

        }
    }
}
