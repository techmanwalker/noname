pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.App
import Player.Fullscreen
import Player.Listings
import Player.LyricsManifest
import Player.LocalLibrary
import Player.PlayerPresenter
import Player.Primitives

Item {
    id: root

    required property bool immersive

    // ── Gradient ───────────────────────────────────────────────────────────
    property real gradientMargin: 50

    readonly property real coverGlobalX: mainRow.x + leftCol.x + nowplaying_cover.x

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

    signal switchToLyricsViewRequested () // request

    // ── Layout ─────────────────────────────────────────────────────────────

    property real songCoverWidth: 48
    property real songCoverHeight: songCoverWidth
    property real songInnerSpacing: 8
    property real songFadePadding: 20

    Row {
        id: mainRow
        spacing: root.songCoverWidth

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

                TapHandler {
                    onTapped: root.switchToLyricsViewRequested ()
                }
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
                        anchors.verticalCenter: parent.verticalCenter
                        
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

            PlayQueueHeader {
                id: metadataContainer
                title:  PlayerPresenter.title
                artist: PlayerPresenter.artist
                album:  PlayerPresenter.album

                Layout.fillWidth: true

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

                Label {
                    text: qsTr("No media playing right now. Browse or drag one or more audio files here to start playing.")

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    wrapMode: Text.WordWrap

                    // MediumLabel
                    font.pointSize: 13
                    font.weight: Font.Medium
                }
            }

            Component {
                id: nextQueue_c

                Playlist {
                    id: nextQueue
                    model: PlayQueue

                    scrollBarWidth: rightCol.scrollBarWidth

                    songCoverWidth: root.songCoverWidth
                    songInnerSpacing: root.songInnerSpacing

                    clip: true
                    reuseItems: true // tons of songs moving

                    onSongClicked: (song) => {
                        PlayQueue.playhead = PlayQueue.index(song.index, 0)
                    }

                    additionalMenuActions: [
                        {
                            text: qsTr("Clear queue"),
                            action: () => PlayQueue.clear()
                        }
                    ]
                }
            }
        }
    }
}