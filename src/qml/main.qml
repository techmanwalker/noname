import QtQuick.Controls
import Player


ApplicationWindow {
    id: root

    visible: true
    title: "Noname -"
    color: "#000"

    FullscreenPlayer {
        id: contents

        anchors.fill: parent
    }
}