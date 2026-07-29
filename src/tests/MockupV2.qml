import QtQuick.Controls
import QtQuick
import Player


ApplicationWindow {
    id: root

    visible: true
    title: "Noname - Playing Now"
    color: "#000"

    width: 950
    height: 650

    FullscreenPlayer {
        id: contents

        anchors.fill: parent
    }
}