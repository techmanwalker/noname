pragma Singleton

import QtQuick

import Player.Fullscreen
import Player.Presenter

QtObject {
    id: root

    // Every binding that reaches lightModeForBounds re-evaluates whenever it is bumped.
    property int epoch: 0

    function invalidate () { root.epoch++; }

    property Connections presenterSignals: Connections {
        target: PlayerPresenter
        function onLumasChanged () { root.invalidate() }
    }

    function lightModeForBounds (bounds: rect, background: Background, presenter: PlayerPresenter, log_item_name: string) : bool {
        // dark by default
        if (bounds.width === 0 || bounds.height === 0) return false;

        // Subscribe to the epoch. Must stay ABOVE every early return,
        // since reads after a return are never tracked.
        void root.epoch;

        // use the multiplier values background uses
        let multiplier = background.lumaMultiplierAtCenter (bounds);

        let m_backdrop = presenter.backdropLumaForRect(bounds) * multiplier;
        let m_ring     = presenter.ringLumaForRect(bounds)     * multiplier;

        /*
        let background_area = background.width    * background.height
        let bounds_area = bounds.width            * bounds.height
        let bounds_background_ratio = bounds_area / Math.max(background_area, 1)
        */

        if (m_backdrop > 0.90) return true;   // always light
        if (m_backdrop < 0.05) return false;  // always dark (redundant while the gate below is 0.48)

        let did_pick_light = m_backdrop > 0.7 && m_ring > 0.6;

        if (log_item_name && log_item_name !== "undefined") {
            console.log(
                "Light mode for " + log_item_name + ":\n"
            +   "bounds: " + bounds + "\n"
            +   "multiplier: " + multiplier.toString() + "\n"
            +   "multiplied backdrop L: " + m_backdrop.toString() + "\n"
            +   "multiplied ring L: " + m_ring.toString() + "\n"
        //  +   "area ratio bounds/bg: " + bounds_background_ratio.toString() + "\n"
            +   "light mode picked: " + (did_pick_light ? "light" : "dark")

            )
        }
        
        return did_pick_light
    }
}