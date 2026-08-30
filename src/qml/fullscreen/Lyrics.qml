pragma ComponentBehavior: Bound
import QtQuick

import Player.PlayerPresenter
import Player.Fullscreen

ListView {
    id: root

    delegate: LyricDelegate {
        required property var model
        required property int index

        timestamp: model.timestamp
        text: model.text
        highlighted: root.model.highlighted === root.model.index(index, 0)

        TapHandler {
            onTapped: PlayerPresenter.position_ms = parent.model.timestamp
        }
    }
}