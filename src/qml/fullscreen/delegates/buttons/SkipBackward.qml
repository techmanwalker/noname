import QtQuick

import Player.Primitives
import Player.PlayerPresenter

ResizableButton {
    icon.name: "media-skip-backward"

    onClicked: PlayerPresenter.prev()
}