import QtQuick

import Player.Effects

Item {
    id: root
    property url  source

    property real maximumWidth: Screen.width
    property real maximumHeight: Screen.height

    // x coordinates from 0 to 1
    // edges points are always 0 and 1 and never changed
    // none of the below represents the edges location
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
        Qt.point(0.00, 1),
        Qt.point(0.15, 1),
        Qt.point(0.20, 0.8),
        Qt.point(0.30, 0.72),
        Qt.point(0.40, 0.7),
        Qt.point(0.50, 0.72),
        Qt.point(0.60, 0.74),
        Qt.point(0.71, 1.3),
        Qt.point(0.75, 1.6),
        Qt.point(0.80, 1),
        Qt.point(0.90, 1.3),
        Qt.point(1.00, 1)
    ]

    // fallback for contentLightness < 0 (unset)
    property real coverBackLumaFallback: 0.65

    // base cover_back_l
    readonly property real implicitMultiplier: root.contentLightness < 0
        ? root.coverBackLumaFallback
        : root.clamp(root.curveValue(root.coverBackLumaStops, root.contentLightness))

    // OKLC value multipliers (from 0 to 1 as well)
    // ensure symmetric gradient
    property real cover_back_l: implicitMultiplier

    // The control points the shader receives, vec3(x, L_mult, C_mult), as ONE value.
    // One binding means one change notification, so the overlay and
    // lumaMultiplierAtCenter never see a half-updated curve.
    readonly property var curve: {
        const back  = root.cover_back_l
        const edges = back * (root.implicitMultiplier < 1 ? 0.4 : 1)   // symmetric gradient
        const inner = back * (root.implicitMultiplier < 1 ? 0.7 : 1.1)
        return [
            Qt.vector3d(0.00,                         edges, edges * 1.4),
            Qt.vector3d(root.clamp(root.playerLeft),  inner, inner * 1.3),
            Qt.vector3d(root.clamp(root.coverLeft),   back,  back  * 1.2),
            Qt.vector3d(root.clamp(root.coverRight),  back,  back  * 1.2),
            Qt.vector3d(root.clamp(root.playerRight), inner, inner * 1.3),
            Qt.vector3d(1.00,                         edges, edges * 1.4)
        ]
    }

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

    // Luma multiplier the overlay shader applies at the horizontal center of
    // `region`. `region` is in this item's own coordinates (x = 0 is
    // Background's left edge); only its x and width matter. Both are divided
    // by this item's real width, not maximumWidth, so they land on the same
    // 0-1 axis the shader uses. The part of the segment outside the item is
    // ignored, so the center is that of the visible part.
    function lumaMultiplierAtCenter (region: rect) : real {
        if (root.width <= 0) return 1; // identity multiplier; nothing is drawn at zero width anyway

        const left  = root.clamp(region.x / root.width);
        const right = root.clamp((region.x + region.width) / root.width);

        // Same curve as evalPoints() in background_overlay.frag, including its
        // floor at 0. The overlay is bound to root.curve too, so they cannot drift.
        return Math.max(0, root.curveValue(root.curve, (left + right) / 2));
    }
    
    Image {
        id: img
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectCrop
        visible: false

        mipmap: true
    }

    BackgroundOverlay {
        id: bgOverlay
        anchors.fill: parent
        source: img
        visible: false

        pointA:  root.curve[0]
        pointPA: root.curve[1]
        pointCA: root.curve[2]
        pointCB: root.curve[3]
        pointPB: root.curve[4]
        pointB:  root.curve[5]
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