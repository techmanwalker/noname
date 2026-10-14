import QtQuick

import Player.Primitives
import Player.PlayerPresenter

ResizableButton {
    icon.name: "media-skip-forward"

    property PlayerPresenter stateModel: PlayerPresenter

    onClicked: stateModel.next()
}