import QtQuick
import QtQuick.Controls

import Player.Primitives

Column {
    id: root

    property string title
    property string artist
    property string album

    property bool lightMode: false

    signal clicked()

    TapHandler {
        onTapped: root.clicked()
    }

    Label {
        text: root.title
        font.weight: Font.DemiBold
        font.pointSize: 20

        color: root.lightMode ? "black" : "white"

        visible: root.title.length > 0

        width: parent.width
        elide: Text.ElideRight

        maximumLineCount: 3
        wrapMode: Text.WordWrap
    }

    Label {
        text: root.artist

        visible: root.artist.length > 0

        width: parent.width
        elide: Text.ElideRight

        Binding on color {
            value: "black"
            when: root.lightMode
        }
    }

    Label {
        text: root.album

        visible: root.album.length > 0

        Binding on color {
            value: "black"
            when: root.lightMode
        }

        width: parent.width
        elide: Text.ElideRight
    }
}