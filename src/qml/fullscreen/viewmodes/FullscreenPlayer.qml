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
    property Background background

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

    function lightModeForBounds (bounds: rect, name: string) : bool {

        // use the multiplier values background uses
        let multiplier = root.background.lumaMultiplierAtCenter (bounds);

        let m_backdrop = PlayerPresenter.backdropLumaForRect(bounds) * multiplier;
        let m_ring     = PlayerPresenter.ringLumaForRect(bounds)     * multiplier;

        // console.log ("name: " + name + " ; multiplier: " + multiplier + " ; multiplied backdrop luma: " + m_backdrop + " ; ring luma: " + m_ring)

        if (m_backdrop > 0.92) return true;   // always light
        if (m_backdrop < 0.05) return false;  // always dark (redundant while the gate below is 0.48)

        return m_backdrop > 0.7 && m_ring > 0.6;
    }

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

                property bool lightMode: PlayerPresenter.lumaSentinel && root.lightModeForBounds(controlsBounds.bounds)

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

                /*
                onBoundsChanged: console.log("controls backing luma: " + PlayerPresenter.backdropLumaForRect(controlsBounds.bounds) + ", "
                    + " ring luma: " + PlayerPresenter.ringLumaForRect(controlsBounds.bounds))
                */
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

                usedInImmersiveMode: root.immersive

                property rect globalBounds: ({
                    x: metadataContainerBounds.x,
                    y: metadataContainerBounds.y,
                    width: metadataContainer.textContentWidth,
                    height: metadataContainer.textContentHeight
                })

                lightMode: PlayerPresenter.lumaSentinel && root.lightModeForBounds(globalBounds)
            }

            // _d = "the drop area"
            // _s = "the stack"
            // _c = "the content"
            // _p = "the placeholder"

            DropArea {
                id: nextQueue_d

                keys: ["text/uri-list"]

                visible: !root.immersive

                Layout.fillHeight: true
                Layout.preferredWidth: (leftCol.width * .6) + (rightCol.scrollBarWidth * 6)

                Layout.alignment: Qt.AlignVCenter

                Layout.topMargin: root.songCoverHeight * .4

                onDropped: (drop) => {
                    if (drop.hasUrls) {
                        PlayQueue.batch_append(drop.urls)

                        drop.acceptProposedAction()
                    }
                }

                StackLayout {
                    id: nextQueue_s

                    anchors.fill: parent

                    // set whatever source component is correct right at boot
                    currentIndex: PlayQueue.count === 0 ? 1 : 0

                    Playlist {
                        id: nextQueue_c
                        model: PlayQueue

                        scrollBarWidth: rightCol.scrollBarWidth

                        songCoverWidth: root.songCoverWidth
                        songInnerSpacing: root.songInnerSpacing

                        clip: true
                        reuseItems: true // tons of songs moving

                        property rect globalBounds: Qt.rect(
                                nextQueue_cBounds.x,
                                nextQueue_cBounds.y,
                                nextQueue_c.contentWidth,
                                nextQueue_c.contentHeight
                            )

                        lightMode: PlayerPresenter.lumaSentinel && root.lightModeForBounds(globalBounds)

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

                    Label {
                        id: nextQueue_p

                        text: qsTr("No media playing right now. Browse or drag one or more audio files here to start playing.")

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment:   Text.AlignVCenter

                        wrapMode: Text.WordWrap

                        // MediumLabel
                        font.pointSize: 13
                        font.weight: Font.Medium

                        property rect globalBounds: Qt.rect(
                                nextQueue_pBounds.x,
                                nextQueue_pBounds.y,
                                nextQueue_p.contentWidth,
                                nextQueue_p.contentHeight
                            )

                        property bool lightMode: PlayerPresenter.lumaSentinel && root.lightModeForBounds(globalBounds)

                        color: lightMode ? Theme.light.texts.primaryText : Theme.dark.texts.primaryText
                    }
                }
            }

            SceneRect {
                id: nextQueue_cBounds
                target: nextQueue_c
            }

            SceneRect {
                id: nextQueue_pBounds
                target: nextQueue_p
            }

            SceneRect {
                id: metadataContainerBounds
                target: metadataContainer
            }
        }
    }
}