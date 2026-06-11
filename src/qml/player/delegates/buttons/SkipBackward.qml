import QtQuick
import QtQuick.Controls

import Player.PlaybackPresentation

ToolButton {
    icon.name: "media-skip-backward"

    onClicked: PlaybackPresentation.prev()
}