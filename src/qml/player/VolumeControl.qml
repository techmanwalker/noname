import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import "delegates"

FlexboxLayout {
    id: root

    direction: FlexboxLayout.Row
    alignItems: FlexboxLayout.AlignCenter

    Layout.fillWidth: false
    Layout.fillHeight: false
    
    AccessibleSlider {
        id: volumeSlider
        orientation: 
            (root.direction === FlexboxLayout.Column
                || root.direction === FlexboxLayout.ColumnReverse
            ) ? Qt.Vertical : Qt.Horizontal
    }

    ToolButton {
        id: muteButton
        icon.name: volumeSlider.value > 0 ? "audio-volume-high" : "audio-volume-muted"
        flat: true
        padding: 0
        
        onClicked: {
            volumeSlider.value = volumeSlider.value > 0 ? 0 : volumeSlider.lastNonZeroValue
        }
    }
}