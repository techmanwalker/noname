import QtQuick
import QtQuick.Layouts

import Player
import PlayerModels
import Primitives

Item {
    id: root

    // ── Gradient ───────────────────────────────────────────────────────────
    property real gradientMargin: 50

    property real coverGlobalX: mainRow.x + leftCol.x + nowplaying_cover.x

    Background {
        source: PlayerState.cover
        anchors.fill: parent

        // Math.max(1, root.width) prevents zero-division errors during
        // the brief moment when the window is still being constructed
        playerLeft:  0
        coverLeft:   root.coverGlobalX                                 / Math.max(1, root.width)
        coverRight:  (root.coverGlobalX + nowplaying_cover.width)      / Math.max(1, root.width)
        playerRight: root.width
    }

    // ── Cover sizing ───────────────────────────────────────────────────────

    // Ideal cover size — large enough to look great on 4K
    readonly property real coverIdealSize: 600

    // Vertical space consumed by controls and margins
    // Reactive: recalculates if controls change height
    readonly property real controlsHeight:  controls.implicitHeight + 40
    readonly property real verticalPadding: 80  // top + bottom breathing room

    // Actual size: shrinks when the window is too small, floats freely otherwise
    readonly property real coverSize: Math.min(
        coverIdealSize,
        root.height - controlsHeight - verticalPadding
    )

    // ── Layout ─────────────────────────────────────────────────────────────
    Row {
        id: mainRow
        spacing: 20

        anchors.centerIn: parent
        anchors.margins: 20

        // Left column: cover + controls
        ColumnLayout {
            id: leftCol
            spacing: basicControls.height / 2

            anchors.verticalCenter: parent.verticalCenter

            Cover {
                id: nowplaying_cover
                source: PlayerState.cover

                Layout.preferredWidth:  root.coverSize
                Layout.preferredHeight: root.coverSize
                Layout.alignment: Qt.AlignHCenter
            }

            ColumnLayout {
                id: controls

                Layout.maximumWidth: nowplaying_cover.width * .75
                Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter

                DurationControl {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true

                    stateModel: PlayerState
                }

                // Bottom bar: volume | playback | shuffle+repeat
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: basicControls.height

                    VolumeControl {
                        anchors.left: parent.left
                        width: 100

                        stateModel: PlayerState
                    }

                    BasicControls {
                        id: basicControls
                        anchors.centerIn: parent
                    }

                    RowLayout {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 5

                        ShuffleButton {
                            Layout.alignment: Qt.AlignVCenter
                        }

                        RepeatButton {
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }
            }
        }

        // Right column: metadata + upcoming queue
        ColumnLayout {
            id: rightColumn
            height: leftCol.height

            // + scrollbar padding
            property int scrollBarWidth: 4
            width: (leftCol.width * .6) + (scrollBarWidth * 6)

            MetadataContainer {
                title:  PlayerState.title
                artist: PlayerState.artist
                album:  PlayerState.album
            }

            Playlist {
                id: nextQueue
                model: NextQueue

                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.fillHeight: true

                scrollBarWidth: rightColumn.scrollBarWidth
            }
        }
    }
}