pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

ScrollBar {
    id: root
    policy: ScrollBar.AsNeeded

    property int barWidth: 4
    property int logicalWidth: barWidth * 8 

    // Helper property to simplify ternary logic
    readonly property bool isVertical: orientation === Qt.Vertical

    // Automatic implicit dimensions based on orientation
    implicitWidth:  isVertical ? logicalWidth : -1
    implicitHeight: isVertical ? -1           : logicalWidth

    topPadding:    0
    bottomPadding: 0
    leftPadding:   0
    rightPadding:  0

    // ── Track (Background) ──────────────────────────────────
    background: Rectangle {
        id: bg
        // Swap dimensions based on orientation
        width:  root.isVertical ? root.barWidth : root.width
        height: root.isVertical ? root.height   : root.barWidth
        
        // Dynamic centering
        x: root.isVertical ? (root.width  - width)  / 2 : 0
        y: root.isVertical ? 0 : (root.height - height) / 2
        
        radius: root.barWidth / 2
        color:  "#40ffffff"
    }

    // ── Handle (Controller) ─────────────────────────────────
    contentItem: Item {
        Rectangle {
            // ScrollBar automatically manages parent.height/width length
            width:  root.isVertical ? root.barWidth : parent.width
            height: root.isVertical ? parent.height : root.barWidth
            
            x: root.isVertical ? (parent.width  - width)  / 2 : 0
            y: root.isVertical ? 0 : (parent.height - height) / 2
            
            radius: root.barWidth / 2
            color:  "#ffffff"
            opacity: root.pressed ? 1.0 : 0.7
        }
    }

    // ── Interaction area ────────────────────────────────────
    containmentMask: Item {
        width:  root.isVertical ? root.logicalWidth : root.width
        height: root.isVertical ? root.height       : root.logicalWidth
        
        x: root.isVertical ? (root.width  - width)  / 2 : 0
        y: root.isVertical ? 0 : (root.height - height) / 2
    }
}