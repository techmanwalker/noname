import QtQuick
import QtQuick.Controls

import Player.Fullscreen
import Player.Presenter
import Player.Primitives

Column {
    id: root

    property string title
    property string artist
    property string album

    property bool usedInImmersiveMode

    property bool lightMode

    // no need to be very precise, used to calculate the backdrop luminance
    readonly property int textContentWidth: Math.max(title.contentWidth, artist.contentWidth, album.contentWidth)
    readonly property int textContentHeight: title.contentHeight + artist.contentHeight + album.contentHeight + spacing * 2

    signal clicked()

    TapHandler {
        onTapped: root.clicked()
    }

    Label {
        id: title

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
        id: artist

        text: root.artist.length > 0 ? root.artist : qsTr("Unknown Artist")

        visible: root.title.length > 0

        width: parent.width
        elide: Text.ElideRight

        Binding on color {
            value: "black"
            when: root.lightMode
        }
    }

    Label {
        id: album

        text: root.album

        visible: root.title.length > 0 && root.album.length > 0

        Binding on color {
            value: "black"
            when: root.lightMode
        }

        width: parent.width
        elide: Text.ElideRight
    }
}