import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

Item {
    id: root
    
    Background {
        source: Player.cover
        anchors.fill: parent
    }

    RowLayout {
        id: contents

        anchors.fill: parent
        anchors.margins: 20

        Item { Layout.fillWidth: true }

        Row {
            spacing: 20

            ColumnLayout {
                Cover {
                    id: nowplaying_cover

                    source: Player.cover
                    Layout.preferredWidth: 350
                    Layout.preferredHeight: 350

                    Layout.alignment: Qt.AlignHCenter
                }

                ColumnLayout {
                    id: controls

                    Layout.alignment: Qt.AlignBottom

                    DurationControl {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: basicControls.height
                        
                        VolumeControl {
                            anchors.left: parent.left
                            width: 100
                        }

                        BasicControls {
                            id: basicControls
                            anchors.centerIn: parent
                        }

                        RowLayout {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 5
                            
                            ShuffleButton {
                                Layout.alignment: Qt.AlignVCenter
                            }

                            RepeatButton {
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }

            }

            ColumnLayout {
                MetadataContainer {
                    title: Player.title
                    artist: Player.artist
                    album: Player.album
                }

                Playlist {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true

                    id: nextQueue
                    model: NextQueue
                }
            }
        }

        Item { Layout.fillWidth: true }
    }
}