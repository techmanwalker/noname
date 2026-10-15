import QtQuick

import Player.Primitives
import Player.Presenter

ResizableButton {
    icon.name: "media-skip-forward"

    property PlayerPresenter stateModel: PlayerPresenter

    onClicked: stateModel.next()
}