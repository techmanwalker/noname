import QtQuick

import Player.Primitives
import Player.PlayerPresenter

ResizableButton {
    icon.name: "media-skip-forward"

    onClicked: PlayerPresenter.next()
}