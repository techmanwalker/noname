import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Primitives

RowLayout {
    id: root

    ToolButton {
        id: muteButton
        icon.name: volumeSlider.value > 0 ? "audio-volume-high" : "audio-volume-muted"
        flat: true
        padding: 0

        Layout.alignment: Qt.AlignVCenter
        
        onClicked: {
            volumeSlider.value = volumeSlider.value > 0 ? 0 : volumeSlider.lastNonZeroValue
        }
    }
    
    AccessibleSlider {
        id: volumeSlider
        autoReset: true

        from: 0
        to: 100
        value: Player.volume

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter

        onMoved: Player.volume = value

        implicitWidth: 0
        
        orientation: Qt.Horizontal
    }
}