import QtQuick
import QtQuick.Controls

Label {
    id: root

    property string name: "Unnamed folder"

    text: name

    font.weight: Font.DemiBold

    padding: 8

    HoverHandler {
        id: hover
    }

    background: Rectangle {
        color: "#1f1f1f"

        width: root.width
        height: root.height

        radius: height / 2

        anchors.centerIn: parent

        opacity: hover.hovered ? 1 : 0
    }
}
