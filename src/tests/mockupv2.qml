import QtQuick.Controls
import QtQuick
import Player


ApplicationWindow {
    id: root

    visible: true
    title: "Noname -"
    color: "#000"

    width: 800
    height: 600

    FullscreenPlayer {
        id: contents

        anchors.fill: parent
    }
}