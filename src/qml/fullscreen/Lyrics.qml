pragma ComponentBehavior: Bound
import QtQuick

import Player.PlayerPresenter
import Player.Primitives
import Player.Fullscreen
import Player.LyricsManifest

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

            delegate: Item {
                id: delegateRoot

                width: root.width
                height: del.height

                required property lyric modelData
                required property int index

                LyricDelegate {
                    id: del

                    model: delegateRoot.modelData

                    // Feeds only the break computation now — the item's
                    // actual on-screen width is implicit, so it's free to
                    // grow past this when highlighted.
                    wrapWidth: parent.width / 10 * 4
                    topPadding: highlighted ? 20 : 10
                    bottomPadding: topPadding

                    anchors.horizontalCenter: parent.horizontalCenter

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    highlighted: root.highlightedRowIndex === delegateRoot.index
                }

                TapHandler {
                    onTapped: PlayerPresenter.position_ms = del.model.timestamp
                }
            }
        }
    }
}