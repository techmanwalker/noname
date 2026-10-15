import QtQuick

import Player.Primitives
import Player.Presenter

ResizableButton {
    icon.name: "media-skip-backward"

    property PlayerPresenter stateModel: PlayerPresenter

    onClicked: stateModel.prev()
}