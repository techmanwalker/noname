import QtQuick
import QtQuick.Shapes

import Player.Primitives

Item {
    id: root

    property bool lightMode: false
    property real arcSweep: 270      // a full 360° ring would show no visible rotation

    implicitWidth: 48
    implicitHeight: 48

    Shape {
        id: ring
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.lightMode ? Theme.light.loadingColor : Theme.dark.loadingColor
            strokeWidth: Theme.sizes.loadingStrokeWidth
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (Math.min(root.width, root.height) - Theme.sizes.loadingStrokeWidth) / 2
                radiusY: radiusX
                startAngle: 0
                sweepAngle: root.arcSweep
            }
        }

        NumberAnimation on rotation {
            from: 0
            to: 360
            duration: 1000
            loops: Animation.Infinite
            paused: !root.visible   // effective visibility: holds the angle, resumes from it
        }
    }
}
