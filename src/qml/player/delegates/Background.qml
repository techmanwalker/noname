import QtQuick

import Shaders

Item {
    id: root
    property url  source

    // x coordinates from 0 to 1
    property real playerLeft: .3
    property real playerRight: .7
    property real coverLeft:   .4
    property real coverRight:  .5

    // OKLC value multipliers (from 0 to 1 as well)
    // ensure symmetric gradient
    property real outer_player_l: 0.15
    property real inner_player_l: 0.40
    property real cover_back_l:   0.60

    property real outer_player_c: 0.20
    property real inner_player_c: 0.55
    property real cover_back_c:   0.9

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

    DualKawaseBlur {
        id: bgBlur
        anchors.fill: parent
        source: img
        passes: 4
        offset: 1.5
        visible: false
    }

    BackgroundOverlay {
        anchors.fill: parent
        source: bgBlur.outputSource

        pointA: Qt.vector4d(
            0.00,
            root.outer_player_l,
            root.outer_player_c,
            0
        )

        pointPA: Qt.vector4d(
            root.clamp(root.playerLeft), 
            root.inner_player_l, // luma multiplier
            root.inner_player_c, // chroma multiplier
            0     // unused
        )
        pointPB: Qt.vector4d(
            root.clamp(root.playerRight), 
            root.inner_player_l, 
            root.inner_player_c, 
            0
        )
        pointCA: Qt.vector4d(
            root.clamp(root.coverLeft),
            root.cover_back_l,
            root.cover_back_c,
            0
        )
        pointCB: Qt.vector4d(
            root.clamp(root.coverRight),
            root.cover_back_l,
            root.cover_back_c,
            0
        )

        pointB: Qt.vector4d(
            1,
            root.outer_player_l,
            root.outer_player_c,
            0
        )
    }
}