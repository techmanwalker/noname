import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Player

RowLayout {
    id: root

    property int currentSeconds: 0
    property int maximumSeconds: 0

    // Helper function to format seconds as mm:ss
    function formatTime(seconds) {
        var m = Math.floor(seconds / 60)
        var s = seconds % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

    Label {
        id: currentTime
        text: root.formatTime(root.currentSeconds)

        Layout.alignment: Qt.AlignVCenter
    }

    AccessibleSlider {
        from: 0
        to: root.maximumSeconds
        value: root.currentSeconds

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
    }

    Label {
        id: maximumTime
        text: root.formatTime(root.maximumSeconds)

        Layout.alignment: Qt.AlignVCenter
    }
}