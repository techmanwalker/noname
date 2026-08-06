import QtQuick
import QtQuick.Controls

ToolButton {
    id: root

    property string iconName

    property bool magnify: false

    property real iconSize: magnify ? 24 : 18

    spacing: magnify ? 12 : 8

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
}