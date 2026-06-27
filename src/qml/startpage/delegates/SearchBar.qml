import QtQuick
import QtQuick.Controls

TextField {
    background: Rectangle {
        anchors.fill: parent
        anchors.centerIn: parent

        radius: height / 2 // perfect round

        color: "#181818"
    }

    height: font.pointSize * 4
    width: font.pointSize * 32

    leftPadding: height / 2 // match the radius
    rightPadding: leftPadding

    placeholderText: "Search on your library…"
    placeholderTextColor: "#afafaf"

    color: "white"

    font.pointSize: 12
    font.weight: Font.Medium
}