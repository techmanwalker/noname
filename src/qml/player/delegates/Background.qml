import QtQuick
import QtQuick.Effects // main defocus

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

    // blur and darken
    MultiEffect {
        source: img
        anchors.fill: parent
        blurEnabled: true
        blur: .8
        blurMax: 64
        brightness: -0.6 // darken
    }
}