import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Player.Primitives

Item {
    id: root

    required property var stateModel

    height: Math.max(muteButton.height, volumeSlider.height)

    ToolButton {
        id: muteButton
        icon.name: volumeSlider.value > 0 ? "audio-volume-high" : "audio-volume-muted"
        flat: true
        padding: 0

        anchors.verticalCenter: parent.verticalCenter
        
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

        anchors.left: muteButton.right
        anchors.right: parent.right

        anchors.verticalCenter: parent.verticalCenter

        onMoved: root.stateModel.volume = value

        implicitWidth: 0
        
        orientation: Qt.Horizontal
    }
}