import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root

    property string title
    property string artist

    property color coverFill

    Row {
        Cover {
            id: cover

            width: 100
            height: 100
            anchors.verticalCenter: parent.verticalCenter

            fill: root.coverFill
        }

        Column {
            id: metadataContainer

            height: parent.height
            anchors.verticalCenter: parent.verticalCenter

            Column {
                id: metadata

                Layout.alignment: Qt.AlignVCenter

                Label {
                    text: root.title
                    font.weight: Font.DemiBold
                    font.pointSize: 18

                    visible: root.title.length > 0
                }

                Label {
                    text: root.artist

                    visible: root.artist.length > 0
                }

            }
        }
    }

    Lyrics {
        Layout.fillWidth: true
    }
}