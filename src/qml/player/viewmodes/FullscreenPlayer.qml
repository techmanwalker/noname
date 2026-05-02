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

        ColumnLayout {
            id: leftcolumn

            Layout.fillWidth: true
            Layout.preferredHeight: parent.height

            PlayingNowCard {
                Layout.alignment: Qt.AlignTop

                title: Player.title || "No song playing"
                artist: Player.artist || "Play your favorite song here!"
                album: Player.album
                cover: Player.cover
            }

            Lyrics {
                Layout.fillWidth: true
            }

            ColumnLayout {
                DurationControl {
                    Layout.alignment: Qt.AlignVCenter
                }

                RowLayout {
                    VolumeControl {
                        Layout.alignment: Qt.AlignVCenter
                        direction: FlexboxLayout.RowReverse
                    }

                    BasicControls {
                        Layout.alignment: Qt.AlignVCenter
                    }

                    ShuffleButton {
                    }

                    RepeatButton {
                    }
                }
            }
        }

        Playlist {    
            Layout.alignment: Qt.AlignTop        
            id: nextQueue
            model: NextQueue
        }
    }
}