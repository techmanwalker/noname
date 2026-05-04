import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property string title
    property string artist
    property url cover
    property int duration: 0  // Duration in seconds
    property int coverWidth: 48
    property int coverHeight: 48

    // Helper function to format seconds as mm:ss
    function formatDuration(seconds) {
        var m = Math.floor(seconds / 60)
        var s = seconds % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

    height: coverHeight * 1.4

    Cover {
        id: coverItem

        width: root.coverWidth
        height: root.coverHeight
        source: root.cover

        anchors.verticalCenter: parent.verticalCenter
    }

    Column {
        id: metadata

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: coverItem.right
        anchors.right: durationContainer.left
        
        anchors.leftMargin: 10

        Label {
            text: root.title.length > 0 ? root.title : "Untitled song"
        }

        Label {
            text: root.artist.length > 0 ? root.artist : "Unknown artist"
        }
    }

    Column {
        id: durationContainer
        
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        Label {
            id: durationLabel
            text: root.duration > 0 ? root.formatDuration(root.duration) : "--:--"
            visible: root.duration > 0
        }
    }
}