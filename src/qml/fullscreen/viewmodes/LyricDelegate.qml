pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.LyricsManifest
import Player.Primitives

Label {
    id: root

    required property lyric model
    required property bool highlighted

    text: model.text

    topPadding: highlighted ? 10 : 0
    bottomPadding: topPadding

    font.pointSize: highlighted ? 25 : 20
    font.weight: highlighted ? Font.DemiBold : Font.Medium

    Binding on color {
        value: "white"
        when: root.highlighted
    }
}