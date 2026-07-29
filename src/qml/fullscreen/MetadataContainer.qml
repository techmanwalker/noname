import QtQuick
import QtQuick.Controls

Column {
    id: root

    property string title
    property string artist
    property string album

    signal clicked()

    TapHandler {
        onTapped: root.clicked()
    }

    Label {
        text: root.title
        font.weight: Font.DemiBold
        font.pointSize: 20

        color: "white"

        visible: root.title.length > 0
    }

    Label {
        text: root.artist

        visible: root.artist.length > 0
    }

    Label {
        text: root.album

        visible: root.album.length > 0

        color: "#afafaf"
    }
}