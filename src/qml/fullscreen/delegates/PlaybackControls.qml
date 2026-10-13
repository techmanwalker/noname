import QtQuick
import QtQuick.Layouts

import Player.Fullscreen
import Player.PlayerPresenter
import Player.PlayQueue
import Player.Primitives

ColumnLayout {
    id: controls

    property PlayerPresenter stateModel: PlayerPresenter
    property PlayQueue queue: PlayQueue
    property Theme theme: Theme

    property bool lightMode: false

    readonly property alias basicControlsHeight: basicControls.height

    DurationControl {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true

        stateModel: controls.stateModel

        accentColor: controls.lightMode ?
            controls.theme.light.slider.accentColor :
            controls.theme.dark.slider.accentColor

        backgroundColor: controls.lightMode ?
            controls.theme.light.slider.backgroundColor :
            controls.theme.dark.slider.backgroundColor
    }

    // Bottom bar: volume | playback | shuffle+repeat
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: basicControls.height

        VolumeControl {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            
            width: 100

            stateModel: controls.stateModel

            buttonLightMode: controls.lightMode

            accentColor: controls.lightMode ?
                controls.theme.light.slider.accentColor :
                controls.theme.dark.slider.accentColor

            backgroundColor: controls.lightMode ?
                controls.theme.light.slider.backgroundColor :
                controls.theme.dark.slider.backgroundColor
        }

        BasicControls {
            id: basicControls
            anchors.centerIn: parent

            lightMode: controls.lightMode
        }

        RowLayout {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5

            ShuffleButton {
                Layout.alignment: Qt.AlignVCenter

                lightMode: controls.lightMode

                onCurrentModeChanged: {
                    let mode = currentMode == ShuffleButton.ShuffleMode.ShuffleTracks
                    controls.queue.shuffle = mode
                }
            }

            RepeatButton {
                Layout.alignment: Qt.AlignVCenter

                lightMode: controls.lightMode
            }
        }
    }
}