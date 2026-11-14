pragma Singleton

import QtQuick

import Player.Theater
import Player.Presenter

QtObject {
    id: root

    // Every binding that reaches lightModeForBounds re-evaluates whenever it is bumped.
    property int epoch: 0

    function invalidate () { root.epoch++; }

    // Backdrop lightness above which dark glyphs read better. Calibrated on rendered pixels.
    readonly property real darkGlyphThreshold: 0.5

    property Connections presenterSignals: Connections {
        target: PlayerPresenter

        // Not invalidate() directly: lightnessLevelsChanged is emitted from inside the presenter's
        // future continuation, and Background's point* cascade runs right behind it.
        // Re-evaluate once, from the event loop, when that has settled. callLater
        // coalesces repeated requests into one pass.
        function onLightnessLevelsChanged () { root.invalidate() }
    }

    function lightModeForBounds (bounds: rect, background: Background, presenter: PlayerPresenter, log_item_name: string) : bool {
        // Subscribe to the epoch. Must stay ABOVE every early return,
        // since reads after a return are never tracked.
        // A void root.epoch sentence is completely inert.
        // Force the reading and the dependency registration
        // preventing dead code elimination.
        if (!presenter.lightnessAwareness ||root.epoch < 0) return false;

        // dark by default
        if (bounds.width === 0 || bounds.height === 0) return false;

        // qmlsc won't read a rect parameter, or pass it to a method, once any
        // method call has happened. Copy the scalars out first and hand each
        // call a fresh rect.
        const x = bounds.x, y = bounds.y, w = bounds.width, h = bounds.height;

        // use the multiplier values background uses
        let multiplier = background.lightnessMultiplierAtCenter (Qt.rect(x, y, w, h));

        const l = presenter.backdropLightnessForRect(Qt.rect(x, y, w, h)) * multiplier;
        const did_pick_light = l > root.darkGlyphThreshold;

        if (log_item_name && log_item_name !== "undefined") {
            console.log(
                "Light mode for " + log_item_name + ":\n"
            +   "bounds: " + Qt.rect(x, y, w, h).toString() + "\n"
            +   "multiplier: " + multiplier + "\n"
            +   "multiplied backdrop L: " + l + "\n"
        //  +   "area ratio bounds/bg: " + bounds_background_ratio.toString() + "\n"
            +   "light mode picked: " + (did_pick_light ? "light" : "dark")

            )
        }

        return did_pick_light
    }
}
