pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.Fullscreen
import Player.Listings
import Player.PlayerPresenter
import Player.PlayQueue
import Player.Primitives

Item {
    id: root

    required property bool immersive
    property bool lightMode: false

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

                mipmap: true
                smooth: true

                // bind to maximum possible size to avoid flicker on resizing
                sourceSize: Qt.size(root.coverIdealSize, root.coverIdealSize)
            }

            ColumnLayout {
                id: controls

                visible: !root.immersive || leftCol_hover.hovered

                Layout.maximumWidth: nowplaying_cover.width * .75
                Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter

                property bool lightMode: (PlayerPresenter.backingLumaForRect(controlsBounds.bounds) > 0.55)
                    && PlayerPresenter.coverMidringLuma > 0.63

                DurationControl {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true

                    stateModel: PlayerPresenter

                    accentColor: controls.lightMode ?
                        Theme.light.slider.accentColor :
                        Theme.dark.slider.accentColor

                    backgroundColor: controls.lightMode ?
                        Theme.light.slider.backgroundColor :
                        Theme.dark.slider.backgroundColor
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

                        buttonLightMode: controls.lightMode

                        accentColor: controls.lightMode ?
                            Theme.light.slider.accentColor :
                            Theme.dark.slider.accentColor

                        backgroundColor: controls.lightMode ?
                            Theme.light.slider.backgroundColor :
                            Theme.dark.slider.backgroundColor
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
                        }

                        RepeatButton {
                            Layout.alignment: Qt.AlignVCenter

                            lightMode: controls.lightMode
                        }
                    }
                }
            }

            SceneRect {
                id: controlsBounds
                target: controls
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
                Layout.leftMargin: 15 // playlist song lateral padding
                Layout.rightMargin: Layout.leftMargin

                onClicked: root.immersive = !root.immersive

                lightMode: (PlayerPresenter.backingLumaForRect(metadataBounds.bounds) > 0.55)
                        && ((!root.immersive) ?
                            (PlayerPresenter.coverMidringLuma > 0.7)
                          : (PlayerPresenter.coverCentricLuma > 0.7)
                        ) // context is everything
            }
            
            SceneRect {
                id: metadataBounds
                target: metadataContainer

                onBoundsChanged: console.log("header", x, y, width, height + "\n" +
                    "luma behind: " + PlayerPresenter.backingLumaForRect(metadataBounds.bounds))
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

                property bool lightMode: (PlayerPresenter.backingLumaForRect(nextQueueBounds.bounds) > 0.55)
                        && PlayerPresenter.coverCentricLuma > 0.7

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
                    verticalAlignment:   Text.AlignVCenter

                    wrapMode: Text.WordWrap

                    // MediumLabel
                    font.pointSize: 13
                    font.weight: Font.Medium

                    color: nextQueue_l.lightMode ? Theme.light.texts.primaryText : Theme.dark.texts.primaryText
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

                    lightMode: nextQueue_l.lightMode

                    onSongClicked: (song) => {
                        PlayQueue.playhead = PlayQueue.find_by_source(song.source)
                    }

                    additionalMenuActions: [
                        {
                            text: qsTr("Clear queue"),
                            action: () => PlayQueue.clear()
                        }
                    ]
                }
            }

            SceneRect {
                id: nextQueueBounds
                target: nextQueue_l
            }
        }
    }
}