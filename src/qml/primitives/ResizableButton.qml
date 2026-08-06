import QtQuick
import QtQuick.Controls

import Player.Primitives

ToolButton {
    id: root

    property string iconName

    property bool magnify: false

    property real iconSize: magnify ? 24 : 18

    spacing: magnify ? 12 : 8

    hoverEnabled: true

    icon.name: root.iconName
    
    icon.width: root.iconSize
    icon.height: root.iconSize

    text: root.text
    
    // MediumLabel
    Binding on font.pointSize {
        value: 13
        when: root.magnify
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
    }
}