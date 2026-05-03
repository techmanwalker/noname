import QtQuick
import QtQuick.Controls

Slider {
    id: root
    from: 0
    to: 100

    property bool  autoReset:          false
    property int   lastNonZeroValue:   65
    property real  trackHeight:        4
    property real  trackOpacity:       1.0

    onValueChanged: {
        if (value > 0 && root.autoReset)
            root.lastNonZeroValue = value
    }

    Item {
        id: interactionRect

        x:      0
        y:      (root.height - height) / 2
        width:  root.width
        height: root.trackHeight * 6
    }

    containmentMask: interactionRect
    

    // ── Track ──────────────────────────────────────────────────────────────
    background: Item {
        x:      root.leftPadding
        y:      root.topPadding + (root.availableHeight - height) / 2
        width:  root.availableWidth
        height: root.trackHeight

        // Full background
        Rectangle {
            anchors.fill: parent
            radius:       height / 2
            color:        "#80ffffff"
            opacity:      root.trackOpacity
        }

        // Progress
        Rectangle {
            width:  root.visualPosition * parent.width
            height: parent.height
            radius: height / 2
            color:  "#ffffff"
            opacity: root.trackOpacity
        }
    }

    // ── Handle ─────────────────────────────────────────────────────────────
    handle: Rectangle {
        x:      root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y:      root.topPadding  + (root.availableHeight - height) / 2
        width:  root.trackHeight * 3
        height: width
        radius: width / 2
        color:  "#ffffff"
    }
}