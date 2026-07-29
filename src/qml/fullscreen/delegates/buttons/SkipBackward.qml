import QtQuick
import QtQuick.Controls

import Player.PlayerPresenter

ToolButton {
    icon.name: "media-skip-backward"

    onClicked: PlayerPresenter.prev()
}