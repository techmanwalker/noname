pragma Singleton

import QtQuick

import Player.Fullscreen
import Player.Presenter

QtObject {
    function lightModeForBounds (bounds: rect, background: Background, presenter: PlayerPresenter, log_item_name: string) : bool {

        // use the multiplier values background uses
        let multiplier = background.lumaMultiplierAtCenter (bounds);

        let m_backdrop = presenter.backdropLumaForRect(bounds) * multiplier;
        let m_ring     = presenter.ringLumaForRect(bounds)     * multiplier;

        if (m_backdrop > 0.92) return true;   // always light
        if (m_backdrop < 0.05) return false;  // always dark (redundant while the gate below is 0.48)

        let did_pick_light = m_backdrop > 0.7 && m_ring > 0.6;

        if (log_item_name && log_item_name !== "undefined") {
            console.log(
                "Light mode for " + log_item_name + ":\n"
            +   "multiplier: " + multiplier.toString() + "\n"
            +   "multiplied backdrop L: " + m_backdrop.toString() + "\n"
            +   "multiplied ring L: " + m_ring.toString() + "\n"
            +   "light mode picked: " + (did_pick_light ? "light" : "dark")

            )
        }
        
        return did_pick_light
    }
}