import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root

    property string title
    property string artist
    property string album

    property url cover: ""

    Row {
        Cover {
            id: cover

            width: 100
            height: 100
            anchors.verticalCenter: parent.verticalCenter

            source: root.cover
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

                Label {
                    text: root.album

                    visible: root.album.length > 0
                }

            }
        }
    }

    Lyrics {
        Layout.fillWidth: true
    }
}