import QtQuick
import QtQuick.Controls

import Player.Primitives

TextField {
    background: ElementBackground {
        anchors.fill: parent
        anchors.centerIn: parent

        radius: height / 2 // perfect round
    }

    height: font.pointSize * 4
    width: font.pointSize * 32

    leftPadding: height / 2 // match the radius
    rightPadding: leftPadding

    placeholderText: qsTr("Search…")
    placeholderTextColor: "#afafaf"

    color: "white"

    font.pointSize: 12
    font.weight: Font.Medium
}