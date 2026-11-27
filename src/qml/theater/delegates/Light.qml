pragma Singleton

import QtQuick

import Player.Theater
import Player.Presenter

QtObject {
    id: root

    // Every binding that reaches lightModeForBounds re-evaluates whenever it is bumped.
    property int epoch: 0

    function invalidate () { root.epoch++; }

    property Connections presenterSignals: Connections {
        target: PlayerPresenter

        // Not invalidate() directly: lightnessLevelsChanged is emitted from inside the presenter's
        // future continuation, and Background's point* cascade runs right behind it.
        // Re-evaluate once, from the event loop, when that has settled. callLater
        // coalesces repeated requests into one pass.
        function onLightnessLevelsChanged () { Qt.callLater(root.invalidate) }
    }

    // Backdrop lightness above which dark glyphs read better. Calibrated on rendered pixels.
    readonly property real darkGlyphThreshold: 0.59
    // inside ±margin neither polarity is clearly better, so follow the neighbours
    readonly property real ambiguityMargin: 0.06

    // multiplied backdrop lightness under bounds
    function backdropLightness (bounds: rect, background: Background, presenter: PlayerPresenter) : real {
        // Subscribe to the epoch.
        // Not ready: no cover on screen yet, so the backdrop is black. Dark by default.
        if (!presenter.lightnessAwareness || root.epoch < 0 || !background.ready) return NaN;

        // dark by default
        if (bounds.width === 0 || bounds.height === 0) return NaN;

        // qmlsc won't read a rect parameter, or pass it to a method, once any
        // method call has happened. Copy the scalars out first and hand each
        // call a fresh rect.
        const x = bounds.x, y = bounds.y, w = bounds.width, h = bounds.height;

        // use the multiplier values background uses
        let multiplier = background.lightnessMultiplierAtCenter (Qt.rect(x, y, w, h));

        return presenter.backdropLightnessForRect(Qt.rect(x, y, w, h)) * multiplier;
    }

    function lightModeForBounds (bounds: rect, background: Background, presenter: PlayerPresenter) : bool {
        return root.backdropLightness(bounds, background, presenter) > root.darkGlyphThreshold;
    }

    /* with logs
    function lightModeForBounds (bounds: rect, background: Background, presenter: PlayerPresenter, log_item_name: string) : bool {
        const l = root.backdropLightness(bounds, background, presenter);

        const did_pick_light = l > root.darkGlyphThreshold

        if (log_item_name && log_item_name !== "undefined") {
            console.log(
                "Light mode for " + log_item_name + ":\n"
            +   "bounds: " + bounds.toString() + "\n"
            +   "multiplied backdrop L: " + l + "\n"
        //  +   "area ratio bounds/bg: " + bounds_background_ratio.toString() + "\n"
            +   "light mode picked: " + did_pick_light

            )
        }

        return did_pick_light;      // NaN is false: white glyphs, as before
    }
    */

    function lightModeWithPrior (bounds: rect, background: Background, presenter: PlayerPresenter, prior: bool) : bool {
        const l = root.backdropLightness(bounds, background, presenter);
        if (isNaN(l)) return false;
        return Math.abs(l - root.darkGlyphThreshold) < root.ambiguityMargin
            ? prior
            : l > root.darkGlyphThreshold;
    }
}
