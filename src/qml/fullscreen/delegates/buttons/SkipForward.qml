import QtQuick
import QtQuick.Controls

import Player.PlayerPresenter

ToolButton {
    icon.name: "media-skip-forward"

    onClicked: PlayerPresenter.next()
}