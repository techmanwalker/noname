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
                    background.color: ({ r: 200 / 255, g: 200 / 255, b: 200 / 255, a: .08 })
                }

                when: root.playing
            },

            State {
                name: "selected"

                PropertyChanges {
                    background.color: ({ r: 160 / 255, g: 160 / 255, b: 160 / 255, a: .3 })
                }

                when: root.selected
            },

            State {
                name: "hovered"

                PropertyChanges {
                    background.color: ({ r: 160 / 255, g: 160 / 255, b: 160 / 255, a: .08 })
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
        outerLeftStop.x: root.outer_leftstop
        outerLeftStop.y: 0

        innerLeftStop.x:  root.inner_leftstop
        innerLeftStop.y: 1.0

        innerRightStop.x: root.inner_rightstop
        innerRightStop.y: 1.0

        outerRightStop.x: root.outer_rightstop
        outerRightStop.y: 0

        switchAxis: false

        visible: root.playing || root.hovered || root.selected
    }
}