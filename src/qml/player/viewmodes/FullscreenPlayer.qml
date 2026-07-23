pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player
import Player.PlayerPresenter
import Player.MediaSequences
import Player.Primitives

Item {
    id: root

    property bool immersive: false // hide all controls and buttons, leave you only with the music

    signal switchView() // to other specific view, currently leaving empty means "switch to fullscreen player"

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

    // ── Navigation ─────────────────────────────────────────────────────────

    LabeledButton {
        id: fullscreenToggle
        iconName: "window-minimize"

        text: "Back"

        opacity: (!root.immersive || hovered) ? 1 : 0 // immersion
        hoverEnabled: true

        anchors.right: parent.right
        anchors.top: parent.top

        onClicked: root.switchView()
    }

    // ── Layout ─────────────────────────────────────────────────────────────

    property real songCoverWidth: 48
    property real songCoverHeight: songCoverWidth
    property real songLateralPadding: 24
    property real songVerticalPadding: 12
    property real songInnerSpacing: 8
    property real songFadePadding: 20

    Row {
        id: mainRow
        spacing: root.songCoverWidth - root.songLateralPadding

        anchors.centerIn: parent
        anchors.margins: 40

        // Left column: cover + controls
        ColumnLayout {
            id: leftCol
            spacing: basicControls.height

            height: nowplaying_cover.height + controls.height + spacing

            anchors.verticalCenter: parent.verticalCenter

            HoverHandler {
                id: leftCol_hover
            }

            Cover {
                id: nowplaying_cover
                source: PlayerPresenter.cover

                Layout.preferredWidth:  root.coverSize
                Layout.preferredHeight: root.coverSize
                Layout.alignment: Qt.AlignHCenter
            }

            ColumnLayout {
                id: controls

                visible: !root.immersive || leftCol_hover.hovered

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

                Layout.leftMargin: root.songLateralPadding

                onClicked: root.immersive = !root.immersive
            }

            // _l = "the loader"
            // _c = "the content"
            // _p = "the placeholder"

            Loader {
                id: nextQueue_l

                // set whatever source component is correct right at boot
                sourceComponent: PlayQueue.count === 0 ? nextQueue_p : nextQueue_c

                visible: !root.immersive

                Layout.fillHeight: true
                Layout.preferredWidth: (leftCol.width * .6) + (parent.scrollBarWidth * 6)

                Layout.alignment: Qt.AlignVCenter

                Layout.topMargin: root.songCoverHeight * .4

                DropArea {
                    anchors.fill: parent

                    keys: ["text/uri-list"]

                    onDropped: (drop) => {
                        if (drop.hasUrls) {
                            PlayQueue.batch_append(drop.urls)

                            drop.acceptProposedAction()
                        }
                    }
                }
            }

            Component {
                id: nextQueue_p

                MediumLabel {
                    text: "No media playing right now. Pick a song or drag an audio file here to start playing."

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    wrapMode: Text.WordWrap
                }
            }

            Component {
                id: nextQueue_c

                Playlist {
                    id: nextQueue
                    model: PlayQueue

                    scrollBarWidth: rightCol.scrollBarWidth

                    songCoverWidth: root.songCoverWidth
                    songLeftPadding: root.songLateralPadding
                    songRightPadding: root.songLateralPadding
                    songTopPadding: root.songVerticalPadding
                    songBottomPadding: root.songVerticalPadding
                    songInnerSpacing: root.songInnerSpacing

                    clip: true
                    reuseItems: true // tons of songs moving

                    onSongClicked: (song) => {
                        PlayQueue.playhead = PlayQueue.index(song.index, 0)
                    }
                }
            }
        }
    }
}