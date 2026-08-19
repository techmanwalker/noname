import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import Player.Primitives

ToolButton {
    id: root

    property string iconName

    property bool magnify: true

    property real iconSize: magnify ? 24 : 18

    property bool filled: false // negative space

    spacing: magnify ? 12 : 8

    hoverEnabled: true

    icon.name: root.iconName
    
    icon.width: root.iconSize
    icon.height: root.iconSize

    text: root.text

    font.pointSize : magnify ? 13: 12
 
    contentItem: IconLabel {
        spacing: root.spacing
        mirrored: root.mirrored
        display: root.display

        icon: root.icon
        defaultIconColor: root.visualFocus ? root.palette.highlight : root.palette.buttonText
        text: root.text
        font: root.font
        color: root.filled ? "black" : defaultIconColor
    }
    
    font.weight: magnify ? Font.Light /*Yagami*/ : Font.Medium

    background: ElementBackground {
        anchors.fill: parent
        anchors.centerIn: parent

        hoverEnabled: root.hoverEnabled
        hovered: root.hovered

        clickable: true // this is a button
        clickEnabled: root.enabled
        down: root.down

        filled: root.filled
    }
}