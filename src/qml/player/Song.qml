import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    property string cover // alias to coverItem image
    property string title
    property string artist
    property string duration
    property int coverWidth: 48
    property int coverHeight: 48

    Cover {
        id: coverItem
        Layout.preferredWidth: root.coverWidth
        Layout.preferredHeight: root.coverHeight
        fill: "#fff"
    }

    Column {
        id: metadata
        
        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitHeight

        Layout.alignment: Qt.AlignVCenter

        Label {
            text: root.title.length > 0 ? root.title : "Untitled song"
        }

        Label {
            text: root.artist.length > 0 ? root.artist : "Unknown artist"
        }
    }

    Column {
        id: durationContainer

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitHeight

        Layout.alignment: Qt.AlignRight

        Label {
            id: duration
            text: root.duration
            visible: root.duration.length > 0
        }
    }
}