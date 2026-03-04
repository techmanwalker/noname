import QtQuick
import QtQuick.Controls

Row {
    id: root
    property string cover // alias to coverItem image
    property string title
    property string artist
    property string duration
    property int coverWidth: 48
    property int coverHeight: 48

    width: coverWidth + metadata.width 
    height: coverHeight

    Cover {
        id: coverItem
        width: root.coverWidth
        height: root.coverHeight
        fill: "#fff"
    }

    Column {
        id: metadata
        
        width: implicitWidth
        height: implicitHeight

        anchors.verticalCenter: parent.verticalCenter

        Label {
            text: root.title.length > 0 ? root.title : "Untitled song"
        }

        Label {
            text: root.artist.length > 0 ? root.artist : "Unknown artist"
        }
    }

    Column {
        id: durationContainer

        width: implicitWidth
        height: implicitHeight
        
        anchors.verticalCenter: parent.verticalCenter

        Label {
            id: duration
            text: root.duration
            visible: root.duration.length > 0
        }
    }
}