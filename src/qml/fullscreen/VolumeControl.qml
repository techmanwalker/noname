import QtQuick
import QtQuick.Layouts

import Player.Primitives

RowLayout {
    id: root

    required property var stateModel
    
    spacing: muteButton.width / 2

    height: Math.max(muteButton.height, volumeSlider.height)

    ResizableButton {
        id: muteButton
        icon.name: volumeSlider.value > 0 ? "audio-volume-high" : "audio-volume-muted"
        flat: true
        padding: 0

        Layout.alignment: Qt.AlignVCenter
        
        onClicked: {
            // The slider is bound to this value so both backend and GUI
            // change volume at a time
            if (root.stateModel.volume > 0) {
                root.stateModel.volume = 0
            } else {
                root.stateModel.volume = volumeSlider.lastNonZeroValue
            }
        }
    }
    
    AccessibleSlider {
        id: volumeSlider
        autoReset: true

        from: 0
        to: 100
        value: root.stateModel.volume

        Layout.fillWidth: true

        Layout.alignment: Qt.AlignVCenter

        onMoved: root.stateModel.volume = value

        implicitWidth: 0
        
        orientation: Qt.Horizontal
    }
}