import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Player.Primitives

RowLayout {
    id: root

    required property var stateModel

    // Helper function to format seconds as mm:ss
    function formatTime(ms) {
        let totalSeconds = Math.floor(ms / 1000)
        let minutes = Math.floor(totalSeconds / 60)
        let seconds = totalSeconds % 60
        return `${minutes}:${seconds.toString().padStart(2, '0')}`
    }

    Label {
        id: currentTime
        text: root.formatTime(root.stateModel.position_ms)

        Layout.alignment: Qt.AlignVCenter
    }

    AccessibleSlider {
        from: 0
        to: root.stateModel.duration_ms
        value: root.stateModel.position_ms

        onMoved: root.stateModel.position_ms = value

        Layout.alignment: Qt.AlignVCenter
        Layout.fillWidth: true
    }

    Label {
        id: maximumTime
        text: root.formatTime(root.stateModel.duration_ms)

        Layout.alignment: Qt.AlignVCenter
    }
}