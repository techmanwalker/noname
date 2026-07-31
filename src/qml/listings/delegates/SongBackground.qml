import QtQuick

import Player.Effects

Item {
    id: root

    // Inyectados desde Song.qml o el componente llamador
    property bool playing: false
    property bool hovered: false

    property real outer_leftstop: 0
    property real inner_leftstop: .2
    property real inner_rightstop: .8
    property real outer_rightstop: 1

    Rectangle {
        id: background
        anchors.fill: parent

        color: root.playing
               ? Qt.rgba(160, 160, 160, .08)
               : (root.hovered ? Qt.rgba(160, 160, 160, .04) : "transparent")

        visible: false
    }

    Noise {
        id: noiseEffect
        source: background
        anchors.fill: background
        visible: false 
        intensity: root.playing ? .04 : 0
    }

    EdgeFade {
        id: edgeFadeEffect
        source: noiseEffect.outputSource
        anchors.fill: parent
        
        // map to the item bounds
        outerLeftStop:  Qt.vector2d(root.outer_leftstop, 0.0)
        innerLeftStop:  Qt.vector2d(root.inner_leftstop, 1.0)
        innerRightStop: Qt.vector2d(root.inner_rightstop, 1.0)
        outerRightStop: Qt.vector2d(root.outer_rightstop, 0.0)
        switchAxis: false

        visible: root.playing || root.hovered
    }
}