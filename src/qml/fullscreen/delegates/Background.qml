import QtQuick

import Player.Effects

Item {
    id: root
    property url  source

    // x coordinates from 0 to 1
    property real playerLeft: .15
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

    function clamp (magnitude) {
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

        pointA: Qt.vector3d(
            0.00,
            root.outer_player_l,
            root.outer_player_c,
        )

        pointPA: Qt.vector3d(
            root.clamp(root.playerLeft), 
            root.inner_player_l, // luma multiplier
            root.inner_player_c, // chroma multiplier
        )
        pointPB: Qt.vector3d(
            root.clamp(root.playerRight), 
            root.inner_player_l, 
            root.inner_player_c
        )
        pointCA: Qt.vector3d(
            root.clamp(root.coverLeft),
            root.cover_back_l,
            root.cover_back_c
        )
        pointCB: Qt.vector3d(
            root.clamp(root.coverRight),
            root.cover_back_l,
            root.cover_back_c
        )

        pointB: Qt.vector3d(
            1,
            root.outer_player_l,
            root.outer_player_c,
        )
    }

    DualKawaseBlur {
        id: blur
        anchors.fill: parent
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
            Screen.height < 720 ? 48 : (
                Screen.height >= 1440 ? 96 :
                    64
            )
        )

        visible: true
        intensity: 0.02
    }
}