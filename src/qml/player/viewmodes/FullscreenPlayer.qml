import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player
import Primitives

Item {
    id: root

    // ── Gradient ───────────────────────────────────────────────────────────
    property real gradientMargin: 50

    property real coverGlobalX: contents.x + mainRow.x + leftCol.x + nowplaying_cover.x

    Background {
        source: Player.cover
        anchors.fill: parent

        // Math.max(1, root.width) prevents zero-division errors during
        // the brief moment when the window is still being constructed
        playerLeft:  (contents.x - root.gradientMargin)                    / Math.max(1, root.width)
        coverLeft:   root.coverGlobalX                                     / Math.max(1, root.width)
        coverRight:  (root.coverGlobalX + nowplaying_cover.width)          / Math.max(1, root.width)
        playerRight: (contents.x + contents.width + root.gradientMargin)   / Math.max(1, root.width)
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
    RowLayout {
        id: contents

        anchors.fill: parent
        anchors.centerIn: parent
        anchors.margins: 20

        Row {
            id: mainRow
            spacing: 20
            Layout.alignment: Qt.AlignCenter

            // Left column: cover + controls
            ColumnLayout {
                id: leftCol
                spacing: basicControls.height / 2

                anchors.verticalCenter: parent.verticalCenter

                Cover {
                    id: nowplaying_cover
                    source: Player.cover

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
                    }

                    // Bottom bar: volume | playback | shuffle+repeat
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: basicControls.height

                        VolumeControl {
                            anchors.left: parent.left
                            width: 100
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
                    title:  Player.title
                    artist: Player.artist
                    album:  Player.album
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
}