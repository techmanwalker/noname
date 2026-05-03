import QtQuick
import QtQuick.Effects // main defocus

import Player

Item {
    id: root
    property url source

    Image {
        id: img
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectCrop // do not stretch
        asynchronous: true
        visible: false // hide to use as effect source
    }

    // blur
    DualKawaseBlur {
        anchors.fill: parent
        source: img
        passes: 4
        offset: 1.5
        // frozen: !windowIsResizing ; to be worked on
    }
}