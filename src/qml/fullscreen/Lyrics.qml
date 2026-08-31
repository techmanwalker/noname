pragma ComponentBehavior: Bound
import QtQuick

import Player.PlayerPresenter
import Player.Fullscreen

ListView {
    id: root

    required property int highlightedRowIndex

    spacing: 20

    delegate: LyricDelegate {
        required property int index

        width: root.width

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        id: del

        highlighted: root.highlightedRowIndex === del.index

        TapHandler {
            onTapped: PlayerPresenter.position_ms = del.model.timestamp
        }
    }
}