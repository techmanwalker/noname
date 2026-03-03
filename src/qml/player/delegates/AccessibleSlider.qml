import QtQuick
import QtQuick.Controls


Slider {
    id: root
    from: 0
    to: 100
    value: 65

    property int lastNonZeroValue: 65

    onValueChanged: {
        if (value > 0) {
            root.lastNonZeroValue = value
        }
    }
}