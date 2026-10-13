pragma Singleton

import QtQuick

import Player.Fullscreen
import Player.PlayerPresenter

QtObject {
    function lightModeForBounds (bounds: rect, background: Background, presenter: PlayerPresenter) : bool {

        // use the multiplier values background uses
        let multiplier = background.lumaMultiplierAtCenter (bounds);

        let m_backdrop = presenter.backdrop_luma_for_rect(bounds) * multiplier;
        let m_ring     = presenter.ring_luma_for_rect(bounds)     * multiplier;

        if (m_backdrop > 0.92) return true;   // always light
        if (m_backdrop < 0.05) return false;  // always dark (redundant while the gate below is 0.48)

        return m_backdrop > 0.7 && m_ring > 0.6;
    }
}