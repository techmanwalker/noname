import QtQuick
import QtQuick.Controls

TextField {
    id: field
    placeholderText: "Search in your library…"
    placeholderTextColor: "#555555"

    leftPadding: bg.radius // radius end and text beginning are aligned

    background: Rectangle {
        id: bg

        color: "#181818"

        height: field.font.pixelSize * 3
        width:  field.font.pixelSize * 20 // about 20 characters

        radius: height / 2

        anchors.verticalCenter: parent.verticalCenter
    }
}