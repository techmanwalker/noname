import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

ApplicationWindow {
    id: root
    visible: true
    title: "Noname -"
    color: "#000"

    width: contents.implicitWidth
    height: contents.implicitHeight

    RowLayout {
        id: contents

        anchors.fill: parent

        Layout.alignment: Qt.AlignLeft

        ColumnLayout {
            Layout.fillWidth: false
            PlayingNowCard {
                title: "How Soon Is Now?"
                artist: "t.A.T.u"
                coverFill: "#e6e6b0"

                Layout.fillHeight: true
            }

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

        Playlist {
            Layout.alignment: Qt.AlignTop | Qt.AlignRight
            
            id: nextQueue
            model: NextQueue
        }
    }
}