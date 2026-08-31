pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.LyricsManifest
import Player.Primitives

Label {
    id: root

    required property lyric model
    required property bool highlighted

    font.pointSize: 24
    font.weight: Font.Medium

    text: model.text

    Binding on color {
        value: "white"
        when: root.highlighted
    }
}