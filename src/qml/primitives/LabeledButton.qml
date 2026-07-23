import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Row {
    id: root

    property string text

    property string iconName
    property real iconSize: 24

    leftPadding: 24
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8
    spacing: 12

    signal clicked()

    property bool hovered
    property bool hoverEnabled: false
    
    HoverHandler {
        onHoveredChanged: root.hovered = hovered
        enabled: root.hoverEnabled
    }

    TapHandler {
        onTapped: root.clicked();
    }

    IconLabel {
        icon.name: root.iconName
        
        icon.width: root.iconSize
        icon.height: root.iconSize

        anchors.verticalCenter: parent.verticalCenter
    }

    MediumLabel {
        text: root.text
        visible: text.length > 0

        anchors.verticalCenter: parent.verticalCenter
    }
}