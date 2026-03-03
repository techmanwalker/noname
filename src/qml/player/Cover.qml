import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    width: 100
    height: width
    property color fill: "#fff"

    color: fill
    Layout.preferredWidth: root.width
    Layout.preferredHeight: root.height
}