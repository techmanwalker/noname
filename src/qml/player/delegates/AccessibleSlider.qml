import QtQuick
import QtQuick.Controls


Slider {
    id: root
    from: 0
    to: 100

    property bool autoReset: false

    property int lastNonZeroValue: 65

    onValueChanged: {
        if (value > 0 && root.autoReset) {
            root.lastNonZeroValue = value
        }
    }
}