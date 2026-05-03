import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

Item {
    id: root
    
    // Adjustable margin between gradient limits and container limits
    property real gradientMargin: 50

    // Calculate global X position of the cover summing X of all its parents
    // This creates a reactive binding: if window is resized, this is recalculated
    property real coverGlobalX: contents.x + mainRow.x + leftCol.x + nowplaying_cover.x

    Background {
        source: Player.cover
        anchors.fill: parent

        // We use Math.max(1, root.width) to avoid QML to trigger errors of
        // "zero division" during the milliseconds when the window is building.

        // contents left (50px) minus the adjustable margin. Will become 0.0 if margin is 20.
        playerLeft: (contents.x - root.gradientMargin) / Math.max(1, root.width) 
        
        // Global position of the cover from 0 to 1
        coverLeft: root.coverGlobalX / Math.max(1, root.width)
        
        // Global position + the width of the cover itself
        coverRight: (root.coverGlobalX + nowplaying_cover.width) / Math.max(1, root.width)
        
        // Extreme right of contents (x + width) + the adjustable margin
        playerRight: (contents.x + contents.width + root.gradientMargin) / Math.max(1, root.width)
    }

    RowLayout {
        id: contents

        anchors.fill: parent
        anchors.margins: 20

        Row {
            id: mainRow
            spacing: 20
            Layout.alignment: Qt.AlignHCenter

            ColumnLayout {
                id: leftCol
                
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
                id: rightcolumn
                height: parent.height

                MetadataContainer {
                    title: Player.title
                    artist: Player.artist
                    album: Player.album
                }

                Playlist {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    id: nextQueue
                    model: NextQueue
                }
            }
        }
    }
}