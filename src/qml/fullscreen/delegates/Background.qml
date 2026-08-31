import QtQuick

import Player.Effects

Item {
    id: root
    property url  source

    property real maximumWidth: Screen.width
    property real maximumHeight: Screen.height

    // x coordinates from 0 to 1
    property real playerLeft:  .15
    property real coverLeft:   .25
    property real coverRight:  .75
    property real playerRight: .85

    // OKLC value multipliers (from 0 to 1 as well)
    // ensure symmetric gradient
    property real outer_player_l: 0.28
    property real inner_player_l: 0.48
    property real cover_back_l:   0.65

    property real outer_player_c: 0.39
    property real inner_player_c: 0.64
    property real cover_back_c:   0.73

    // allow disabling the darkener for testing
    property bool darken: true
    property bool blur: true

    function clamp (magnitude: real) : real {
        if (magnitude < 0) return 0;
        if (magnitude > 1) return 1;
        return magnitude;
    }

    Image {
        id: img
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectCrop
        visible: false

        mipmap: false
        onStatusChanged: {
            if (status === Image.Ready) {
                mipmap = true;
            } else if (status === Image.Null || status === Image.Error) {
                mipmap = false;
            }
        }
    }

    BackgroundOverlay {
        id: bgOverlay
        anchors.fill: parent
        source: img
        visible: false

        pointA.x: 0.00
        pointA.y: root.outer_player_l
        pointA.z: root.outer_player_c

        pointPA.x: root.clamp(root.playerLeft)
        pointPA.y: root.inner_player_l // luma multiplier
        pointPA.z: root.inner_player_c // chroma multiplier
        
        pointPB.x: root.clamp(root.playerRight)
        pointPB.y: root.inner_player_l
        pointPB.z: root.inner_player_c
        
        pointCA.x: root.clamp(root.coverLeft)
        pointCA.y: root.cover_back_l
        pointCA.z: root.cover_back_c
        
        pointCB.x: root.clamp(root.coverRight)
        pointCB.y: root.cover_back_l
        pointCB.z: root.cover_back_c

        pointB.x: 1.00
        pointB.y: root.outer_player_l
        pointB.z: root.outer_player_c
    }

    DualKawaseBlur {
        id: blur
        anchors.fill: parent
        maximumWidth: root.maximumWidth
        maximumHeight: root.maximumHeight
        source: root.darken ? bgOverlay.outputSource : img
        passes: 4
        offset: 1.5
        visible: false
    }

    Noise {
        source: root.blur ? blur.outputSource : (
            root.darken ? bgOverlay.outputSource : img
        )

        anchors.fill: parent

        // based on screen size
        seedSize: (
            Screen.height < 720 ? 96 : (
                Screen.height >= 1440 ? 192 :
                    144
            )
        )

        visible: true
        intensity: 0.04
    }
}