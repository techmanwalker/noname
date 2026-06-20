import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Row {
    id: root

    property string text

    property string iconName: ""
    property real iconSize: 24

    leftPadding: 24
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8
    spacing: 12

    IconLabel {
        icon.name: root.iconName
        
        icon.width: root.iconSize
        icon.height: root.iconSize

        anchors.verticalCenter: parent.verticalCenter
    }

    Label {
        text: root.text
        visible: root.text.length > 0

        font.pointSize: 13
        font.weight: Font.Light // Yagami

        anchors.verticalCenter: parent.verticalCenter
    }
}