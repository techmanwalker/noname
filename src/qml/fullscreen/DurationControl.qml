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

    readonly property int pollHz: 15
    readonly property int pollIntervalMs: Math.round(1000 / pollHz)

    property bool pollWhileNotVisible: false // enable sharing the poll with another item even when not visible

    // Last successful poll of the playback position (ms). The initial value is a
    // one-shot snapshot; the first assignment in poll() replaces that binding.
    property real position_ms: stateModel.position_ms()

    function poll(): void {
        if (slider.pressed) // the user owns the handle while scrubbing
            return

        const polled = stateModel.position_ms()

        // After a seek the engine keeps reporting the old position until the new audio
        // reaches the output. position_ms still holds the seek target, so keep it until
        // the engine catches up (or the guard expires).
        if (seekGuard.running) {
            if (Math.abs(polled - position_ms) > slider.toleranceMs) {
                console.log("debug: stale poll ignored after seek", polled, position_ms) // temporary
                return
            }
            seekGuard.stop()
        }

        position_ms = polled

        if (polled < stateModel.duration_ms) {
            sweep.restart()
        } else {
            sweep.stop()
            if (polled > stateModel.duration_ms)
                console.log("debug: reported position is greater than track's duration")
        }
    }

    Timer {
        id: pollTimer
        interval: root.pollIntervalMs
        repeat: true
        triggeredOnStart: true // sync immediately when playback/visibility resumes
        running: (root.visible || root.pollWhileNotVisible)
                 && root.stateModel.playbackState === PlayerPresenter.PlaybackState.playing
        onRunningChanged: if (!running) sweep.stop()
        onTriggered: root.poll()
    }

    // Active between a committed seek and the engine reporting a position near its target.
    // Expiry is the escape hatch if that never happens (seek clamped, track ended, ...).
    Timer {
        id: seekGuard
        interval: 1000
    }

    // Linear sweep from the last poll to where the next poll is expected to land.
    // Runs on the animation driver (vsync), independent of the poll timer.
    NumberAnimation {
        id: sweep
        target: slider
        property: "value"
        from: root.position_ms
        to: root.position_ms + root.pollIntervalMs
        duration: root.pollIntervalMs
        easing.type: Easing.Linear
    }

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

        Layout.preferredWidth: font.pointSize * text.length // stop variable width font shifting the layout
        Layout.alignment: Qt.AlignVCenter

        visible: !root.hideTimestamps

        color: root.accentColor
    }

    AccessibleSlider {
        id: slider

        from: 0
        to: root.stateModel.duration_ms
        value: root.position_ms

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
            root.stateModel.seekTo(value)
            root.position_ms = value // every writer of slider.value now agrees with the seek
            seekGuard.restart()
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
            if (pressed) {
                sweep.stop()
                anchorValue = root.stateModel.position_ms()
            } else {
                settleTimer.stop()

                if (value !== anchorValue) {
                    if (isWithinTolerance()) {
                        // Too small to be a seek: snap the handle back to the playback
                        // position. Assigning breaks the binding, so restore it.
                        value = Qt.binding(function() { return root.position_ms })
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

        Layout.preferredWidth: font.pointSize * text.length // stop variable width font shifting the layout
        Layout.alignment: Qt.AlignVCenter

        horizontalAlignment: Text.AlignRight

        visible: !root.hideTimestamps

        color: root.accentColor
    }
}