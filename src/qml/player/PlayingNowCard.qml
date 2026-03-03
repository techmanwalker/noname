import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root

    property string title
    property string artist

    property color coverFill

    Layout.alignment: Qt.AlignHCenter

    Cover {
        Layout.alignment: Qt.AlignHCenter

        fill: root.coverFill
    }

    ColumnLayout {
        Label {
            text: root.title
            font.weight: Font.DemiBold
            font.pointSize: 18

            visible: root.title.length > 0

            Layout.fillWidth: true
        }

        Label {
            text: root.artist

            visible: root.artist.length > 0

            Layout.fillWidth: true
        }
    }
}