import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root

    property string title
    property string artist
    property string album

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