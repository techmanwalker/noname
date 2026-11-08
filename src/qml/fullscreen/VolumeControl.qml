import QtQuick
import QtQuick.Layouts

import Player.Presenter
import Player.Primitives

RowLayout {
    id: root

    required property PlayerPresenter stateModel
    
    spacing: muteButton.width / 3

    property color accentColor: Theme.dark.slider.accentColor
    property color backgroundColor: Theme.dark.slider.backgroundColor

    property bool buttonLightMode: false
    property alias buttonPadding: muteButton.padding

    readonly property alias muteButtonWidth: muteButton.width

    ResizableButton {
        id: muteButton
        icon.name: volumeSlider.value > 0 ? "audio-volume-high" : "audio-volume-muted"
        flat: true

        Layout.alignment: Qt.AlignVCenter

        lightMode: root.buttonLightMode
        
        onClicked: PlayerPresenter.toggleMute()
    }
    
    AccessibleSlider {
        id: volumeSlider
        autoReset: true

        from: 0
        to: 100
        value: root.stateModel.volume

        logicalHeight: root.height

        Layout.fillWidth: true

        Layout.alignment: Qt.AlignVCenter

        onMoved: root.stateModel.volume = value

        implicitWidth: 0
        
        orientation: Qt.Horizontal

        accentColor: root.accentColor
        backgroundColor: root.backgroundColor
    }

    height: Math.max(muteButton.height, volumeSlider.height)
}