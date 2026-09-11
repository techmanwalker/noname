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

    // after
    property real contentLightness: -1 /* from 0 to 1 */

    // Curve stops (ascending x), mapping contentLightness -> cover_back_l.
    // Both axes range 0 to 1. Non-monotonic (a hump, not an inverse);
    // cover_back_l below walks these as a Catmull-Rom-style cubic 
    // Hermite spline. Tune freely.

    property var coverBackLumaStops: [
        Qt.point(0.00, 0.50),
        Qt.point(0.15, 0.75),
        Qt.point(0.20, 0.70),
        Qt.point(0.30, 0.50),
        Qt.point(0.40, 0.70),
        Qt.point(0.50, 0.60),
        Qt.point(0.60, 0.55),
        Qt.point(0.71, 0.65),
        Qt.point(0.75, 0.50),
        Qt.point(0.80, 0.60),
        Qt.point(0.90, 0.50),
        Qt.point(1.00, 0.40)
    ]

    // Used verbatim whenever contentLightness is the "unset" sentinel
    // (< 0) — e.g. this component previewed standalone, with no real
    // coverLuma ever bound to it. Matches the old fixed cover_back_l.
    property real coverBackLumaFallback: 0.65

    // OKLC value multipliers (from 0 to 1 as well)
    // ensure symmetric gradient
    property real outer_player_l: root.cover_back_l * 0.4
    property real inner_player_l: root.cover_back_l * 0.7
    property real cover_back_l:   root.contentLightness < 0
        ? root.coverBackLumaFallback
        : root.clamp(root.curveValue(root.coverBackLumaStops, root.contentLightness))
        
    property real outer_player_c: root.outer_player_l * 1.4
    property real inner_player_c: root.inner_player_l * 1.3
    property real cover_back_c:   root.cover_back_l   * 1.2

    // allow disabling the darkener for testing
    property bool darken: true
    property bool blur: true

    function clamp (magnitude: real) : real {
        if (magnitude < 0) return 0;
        if (magnitude > 1) return 1;
        return magnitude;
    }

        // Cubic Hermite spline through `stops` (ascending-x Qt.point list),
    // with Catmull-Rom-style finite-difference tangents — smooth, and
    // passes exactly through each stop's y at its x. x outside the
    // stops' own domain is held flat at the nearest edge stop rather
    // than extrapolated, so a curve tuned for [0.1, 1.0] can't blow up
    // for stray values below 0.1.
    function curveValue (stops: var, x: real) : real {
        const n = stops.length;
        if (n === 0) return 0;
        if (n === 1) return stops[0].y;

        const cx = Math.min(Math.max(x, stops[0].x), stops[n - 1].x);

        let i = 0;
        while (i < n - 2 && cx > stops[i + 1].x) {
            i++;
        }

        const p0 = stops[Math.max(i - 1, 0)];
        const p1 = stops[i];
        const p2 = stops[i + 1];
        const p3 = stops[Math.min(i + 2, n - 1)];

        const h = p2.x - p1.x;
        const t = h === 0 ? 0 : (cx - p1.x) / h;

        // Duplicate endpoints (p0 === p1, or p3 === p2, on the boundary
        // segments) collapse these into the correct one-sided difference
        // automatically — no special-casing needed.
        const m1 = (p2.y - p0.y) / ((p2.x - p0.x) || h);
        const m2 = (p3.y - p1.y) / ((p3.x - p1.x) || h);

        const t2 = t * t;
        const t3 = t2 * t;
        const h00 =  2 * t3 - 3 * t2 + 1;
        const h10 =      t3 - 2 * t2 + t;
        const h01 = -2 * t3 + 3 * t2;
        const h11 =      t3 -     t2;

        return h00 * p1.y + h10 * h * m1 + h01 * p2.y + h11 * h * m2
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