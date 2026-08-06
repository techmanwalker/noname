import QtQuick

import Player.Effects

Item {
    id: root

    // Inyectados desde Song.qml o el componente llamador
    property bool playing: false
    property bool hovered: false
    property bool selected: false

    property real outer_leftstop: 0
    property real inner_leftstop: .2
    property real inner_rightstop: .8
    property real outer_rightstop: 1

    Rectangle {
        id: background
        anchors.fill: parent

        color: "transparent"

        states: [
            State {
                name: "playing"

                PropertyChanges {
                    background.color: Qt.rgba(200, 200, 200, .08)
                }

                when: root.playing
            },

            State {
                name: "selected"

                PropertyChanges {
                    background.color: Qt.rgba(160, 160, 160, .2)
                }

                when: root.selected
            },

            State {
                name: "hovered"

                PropertyChanges {
                    background.color: Qt.rgba(160, 160, 160, .04)
                }

                when: root.hovered
            }
        ]

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

        visible: root.playing || root.hovered || root.selected
    }
}