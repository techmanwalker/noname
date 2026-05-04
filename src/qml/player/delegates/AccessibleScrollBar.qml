import QtQuick
import QtQuick.Controls

ScrollBar {
    id: root
    policy: ScrollBar.AsNeeded

    property int barWidth: 4
    property int logicalWidth: barWidth * 8 // actual space it takes and that is interactive

    implicitWidth: logicalWidth  // enough space for the containmentMask

    topPadding:    0
    bottomPadding: 0
    leftPadding:   0
    rightPadding:  0

    // ── Track ──────────────────────────────────────────────
    background: Rectangle {
        id: bg
        width:  root.barWidth
        implicitWidth: root.barWidth
        height: root.height
        radius: width / 2
        color:  "#40ffffff"
        x:      (root.width - root.barWidth) / 2
    }

    // ── Handle ─────────────────────────────────────────────
    contentItem: Item {
        Rectangle {
            width:  root.barWidth
            height: parent.height
            x:      (parent.width - root.barWidth) / 2
            radius: width / 2
            color:  "#ffffff"
            opacity: root.pressed ? 1.0 : 0.7
        }
    }

    // ── Interaction area: full height, 6x wider than bar ───
    containmentMask: Item {
        width:  root.logicalWidth
        height: root.height
        x:      (root.width - width) / 2  // centered on the bar
        y:      0
    }
}