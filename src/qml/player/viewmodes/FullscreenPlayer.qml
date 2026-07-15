import QtQuick
import QtQuick.Layouts

import Player
import Player.PlayerPresenter
import Player.MediaSequences
import Player.Primitives

Item {
    id: root

    // ── Gradient ───────────────────────────────────────────────────────────
    property real gradientMargin: 50

    property real coverGlobalX: mainRow.x + leftCol.x + nowplaying_cover.x

    Background {
        source: PlayerPresenter.cover
        anchors.fill: parent

        // Math.max(1, root.width) prevents zero-division errors during
        // the brief moment when the window is still being constructed
        playerLeft:  0
        coverLeft:   root.coverGlobalX                                 / Math.max(1, root.width)
        coverRight:  (root.coverGlobalX + nowplaying_cover.width)      / Math.max(1, root.width)
        playerRight: 1
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
        spacing: nextQueue.songCoverWidth - nextQueue.songLeftPadding

        anchors.centerIn: parent
        anchors.margins: 40

        // Left column: cover + controls
        ColumnLayout {
            id: leftCol
            spacing: basicControls.height

            anchors.verticalCenter: parent.verticalCenter

            Cover {
                id: nowplaying_cover
                source: PlayerPresenter.cover

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

                    stateModel: PlayerPresenter
                }

                // Bottom bar: volume | playback | shuffle+repeat
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: basicControls.height

                    VolumeControl {
                        anchors.left: parent.left
                        width: 100

                        stateModel: PlayerPresenter
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

        // enables pushing the metadata container to the vertical
        // center when the play queue is not visible
        ColumnLayout {
            id: rightCol
            height: leftCol.height

            // + scrollbar padding
            property int scrollBarWidth: 4
            width: (leftCol.width * .6) + (scrollBarWidth * 6)

            MetadataContainer {
                id: metadataContainer
                title:  PlayerPresenter.title
                artist: PlayerPresenter.artist
                album:  PlayerPresenter.album

                Layout.fillWidth: true

                Layout.leftMargin: nextQueue.songLeftPadding

                onClicked: nextQueue.visible = !nextQueue.visible
            }

            Playlist {
                id: nextQueue
                model: PlayQueue

                scrollBarWidth: rightCol.scrollBarWidth

                clip: true
                reuseItems: true // tons of songs moving

                Layout.fillHeight: true
                Layout.preferredWidth: (leftCol.width * .6) + (scrollBarWidth * 6)

                Layout.alignment: Qt.AlignVCenter

                Layout.topMargin: nextQueue.songCoverHeight * .4
            }
        }
    }
}