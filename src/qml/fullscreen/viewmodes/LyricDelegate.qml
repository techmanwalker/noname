pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.Primitives

Label {
    id: root

    // 64 bit qulonglong, be careful
    required property var timestamp

    required property bool highlighted

    font.pointSize: 24
    font.weight: Font.Medium

    Binding on color {
        value: "white"
        when: root.highlighted
    }
}