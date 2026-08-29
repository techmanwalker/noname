pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T

// for some reason, aot gives problems with non template types
T.Label {
    id: root

    property string name: "Unnamed folder"

    text: root.name

    color: root.palette.windowText
    linkColor: root.palette.link

    font {
        weight: Font.DemiBold
    }

    padding: 8

    HoverHandler {
        id: hover
    }

    background: Rectangle {
        color: "#1f1f1f"
        radius: height / 2
        opacity: hover.hovered ? 1 : 0
        anchors.fill: parent
    }
}