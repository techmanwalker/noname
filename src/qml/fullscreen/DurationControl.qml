import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Player.Primitives
import Player.Presenter

RowLayout {
    id: root

    required property PlayerPresenter stateModel

    property bool hideTimestamps: false

    property color accentColor: Theme.dark.slider.accentColor
    property color backgroundColor: Theme.dark.slider.backgroundColor

    // Helper function to format seconds as mm:ss
    function formatTime(ms: double): string {
        let totalSeconds = Math.floor(ms / 1000)
        let minutes = Math.floor(totalSeconds / 60)
        let seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    Label {
        id: currentTime
        text: root.formatTime(slider.value) // passive

        font.weight: Font.Medium

        Layout.alignment: Qt.AlignVCenter

        visible: !root.hideTimestamps

        color: root.accentColor
    }

    AccessibleSlider {
        id: slider

        from: 0
        to: root.stateModel.duration_ms
        value: root.stateModel.position_ms

        // Minimum distance (ms) from the anchor for a drag to count as a seek.
        readonly property int toleranceMs: 500

        // Reference position: playback position at press, then the last committed value.
        property real anchorValue: 0

        function isWithinTolerance() {
            return Math.abs(value - anchorValue) <= toleranceMs
        }

        function commitPosition() {
            if (isWithinTolerance()) {
                return;
            }
            root.stateModel.position_ms = value
            anchorValue = value
        }

        // Fires once the handle has been held still for a given time (ms) while pressed.
        // Re-armed by every user-driven movement, so it can fire repeatedly
        // during a single press.
        Timer {
            id: settleTimer
            interval: 50
            repeat: false
            onTriggered: slider.commitPosition()
        }

        onMoved: {
            if (pressed)
                settleTimer.restart()
        }

        onPressedChanged: {
            root.stateModel.notifySliderPressedChange(pressed);

            if (pressed) {
                anchorValue = root.stateModel.position_ms
            } else {
                settleTimer.stop()

                if (value !== anchorValue) {
                    if (isWithinTolerance()) {
                        // Too small to be a seek: snap the handle back to the playback
                        // position. Assigning breaks the binding, so restore it.
                        value = Qt.binding(function() { return root.stateModel.position_ms })
                    } else {
                        commitPosition()
                    }
                }
            }
        }

        accentColor: root.accentColor
        backgroundColor: root.backgroundColor

        Layout.alignment: Qt.AlignVCenter
        Layout.fillWidth: true
    }

    Label {
        id: maximumTime
        text: root.formatTime(root.stateModel.duration_ms)

        font.weight: Font.Medium

        Layout.alignment: Qt.AlignVCenter

        visible: !root.hideTimestamps

        color: root.accentColor
    }
}