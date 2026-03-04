import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    property string cover
    property string title
    property string artist
    property int duration: 0  // Duration in seconds
    property int coverWidth: 48
    property int coverHeight: 48

    // Helper function to format seconds as mm:ss
    function formatDuration(seconds) {
        var m = Math.floor(seconds / 60)
        var s = seconds % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

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
            id: durationLabel
            text: root.duration > 0 ? root.formatDuration(root.duration) : "--:--"
            visible: root.duration > 0
        }
    }
}