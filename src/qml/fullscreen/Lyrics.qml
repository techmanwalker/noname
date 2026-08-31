pragma ComponentBehavior: Bound
import QtQuick

import Player.PlayerPresenter
import Player.Primitives
import Player.Fullscreen

Loader {
    id: root

    required property var model
    required property int highlightedRowIndex

    sourceComponent: (model.count > 0) ? lyrics_c : lyrics_p
    
    signal switchToPlayerViewRequested ()

    Component {
        id: lyrics_p

        Label {
            text: qsTr("No lyrics.")

            font.pointSize: 32
            font.weight: Font.Light

            opacity: 0.4

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:   Text.AlignVCenter

            TapHandler {
                onTapped: root.switchToPlayerViewRequested()
            }
        }
    }

    Component {
        id: lyrics_c

        ListView {
            model: root.model

            spacing: 20

            delegate: LyricDelegate {
                required property int index

                width: root.width

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                id: del

                highlighted: root.highlightedRowIndex === del.index

                TapHandler {
                    onTapped: PlayerPresenter.position_ms = del.model.timestamp
                }
            }
        }
    }
}