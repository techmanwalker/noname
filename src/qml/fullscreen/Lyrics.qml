pragma ComponentBehavior: Bound
import QtQuick

import Player.PlayerPresenter
import Player.Fullscreen

ListView {
    id: root

    required property int highlightedRowIndex

    delegate: LyricDelegate {
        required property int index

        id: del

        highlighted: root.highlightedRowIndex === del.index

        TapHandler {
            onTapped: PlayerPresenter.position_ms = del.model.timestamp
        }
    }
}