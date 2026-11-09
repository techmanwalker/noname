pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Player.Browser
import Player.Theater
import Player.Listings
import Player.MediaTypes
import Player.Presenter
import Player.PlayQueue
import Player.Primitives

Row {
    id: root

    required property bool immersive
    property Background background
    required property real availableHeight // granted by the parent view, never derived from this item's own size

    property PlayerPresenter stateModel: PlayerPresenter

    // name -> stack index, one entry per TabPage in queueAndMetadata_s: tabs.<name>
    readonly property var tabs: {
        const map = {};
        const pages = queueAndMetadata_s.children;
        for (let i = 0; i < pages.length; ++i) {
            const page = pages[i] as TabPage;
            if (page)
                map[page.name] = i;
        }
        return Object.freeze(map);
    }

    // ── Gradient ───────────────────────────────────────────────────────────
    property real gradientMargin: 50

    readonly property alias coverGlobalBounds: nowplaying_cover_bounds.bounds

    // ── Cover sizing ───────────────────────────────────────────────────────

    // Ideal cover size — large enough to look great on 4K
    readonly property real coverIdealSize: 600

    // Vertical space consumed by controls and margins
    // Reactive: recalculates if controls change height
    readonly property real controlsHeight:  controls.implicitHeight + 40
    readonly property real verticalPadding: 80  // top + bottom breathing room

    
    // Actual size: shrinks when the window is too small, floats freely otherwise
    readonly property real coverSize: Math.max(0, Math.min(
        coverIdealSize,
        availableHeight - controlsHeight - verticalPadding
    ))

    signal switchToLyricsViewRequested () // request

    // ── Layout ─────────────────────────────────────────────────────────────

    property real songCoverWidth: 48
    property real songCoverHeight: songCoverWidth
    property real songInnerSpacing: 8
    property real songFadePadding: 20

    spacing: root.songCoverWidth

    // Left column: cover + controls
    ColumnLayout {
        id: leftCol
        spacing: controls.basicControlsHeight

        height: nowplaying_cover.height + controls.height + spacing

        anchors.verticalCenter: parent.verticalCenter

        HoverHandler {
            id: leftCol_hover
        }

        Cover {
            id: nowplaying_cover
            source: root.stateModel.cover

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

        SceneRect {
            id: nowplaying_cover_bounds
            target: nowplaying_cover
        }

        PlaybackControls {
            id: controls

            lightMode: Light.lightModeForBounds(controlsBounds.bounds, root.background, PlayerPresenter)

            visible: !root.immersive || leftCol_hover.hovered

            Layout.maximumWidth: Math.min(nowplaying_cover.width * .75 + 2 * buttonPadding, nowplaying_cover.width)
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
        }

        SceneRect {
            id: controlsBounds
            target: controls

            /*
            onBoundsChanged: console.log("controls backing lightness: " + root.stateModel.backdropLightnessForRect(controlsBounds.bounds) + ", "
                + " ring lightness: " + root.stateModel.ringLightnessForRect(controlsBounds.bounds))
            */
        }
    }

    // Right column: metadata + upcoming queue

    // It is independent of immersive
    // mode and of whether the queue is shown.
    DropArea {
        id: rightCol_d

        keys: ["text/uri-list"]

        width: rightCol.width
        height: rightCol.height

        onDropped: (drop) => {
            if (drop.hasUrls) {
                PlayQueue.batchAppend(drop.urls)

                drop.acceptProposedAction()
            }
        }

        // Enables pushing the metadata container to the vertical
        // center when the play queue is not visible
        ColumnLayout {
            id: rightCol
            height: leftCol.height

            // + scrollbar padding
            property int scrollBarWidth: 4
            width: (leftCol.width * .6) + (scrollBarWidth * 6)

            spacing: root.songCoverHeight * .2

            PlayQueueHeader {
                id: metadataContainer
                title:  root.stateModel.title
                artist: root.stateModel.artist
                album:  root.stateModel.album

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

                lightMode: Light.lightModeForBounds(globalBounds, root.background, PlayerPresenter)
            }

            Row {
                id: queueAndMetadataTabs 

                visible: queueAndMetadata_s.visible && !queueAndMetadata_s.is_showing_no_tracks_placeholder
                Layout.leftMargin: metadataContainer.Layout.leftMargin
                                   - (tabButtons.count > 0 ? tabButtons.itemAt(0).leftPadding : 0)

                property bool lightMode: Light.lightModeForBounds(queueAndMetadataTabsBounds.bounds, root.background, PlayerPresenter)

                Repeater {
                    id: tabButtons

                    // one button per page, minus the placeholder
                    model: {
                        const pages = [];
                        const stack = queueAndMetadata_s.children;
                        for (let i = 0; i < stack.length; ++i) {
                            const page = stack[i] as TabPage;
                            if (page && i !== root.tabs.placeholder)
                                pages.push(page);
                        }
                        return pages;
                    }

                    ResizableButton {
                        id: tabButton

                        required property TabPage modelData
                        readonly property int stackIndex: root.tabs[modelData.name]

                        icon.name: modelData.iconName
                        text: modelData.title
                        flat: true
                        magnify: false

                        font.weight: queueAndMetadata_s.currentIndex === stackIndex ? Font.Medium : Font.Light

                        lightMode: queueAndMetadataTabs.lightMode

                        onClicked: queueAndMetadata_s.currentIndex = stackIndex
                    }
                }
            }

            SceneRect {
                id: queueAndMetadataTabsBounds
                target: queueAndMetadataTabs
            }

            // _s = "the stack"
            // _c = "the content"
            // _p = "the placeholder"
            // _d = "the current track details"

            StackLayout {
                id: queueAndMetadata_s

                visible: !root.immersive

                Layout.fillHeight: true
                Layout.preferredWidth: (leftCol.width * .6) + (rightCol.scrollBarWidth * 6)

                Layout.alignment: Qt.AlignVCenter

                readonly property bool is_showing_no_tracks_placeholder: queueAndMetadata_s.currentIndex === root.tabs.placeholder

                // The placeholder is only shown while the queue is empty, and leaving it picks the
                // default tab for the current count. Anything else is the user's own tab choice.
                // (A `currentIndex:` binding can't express this: the first tab click assigns the
                // property imperatively and destroys the binding.)
                function syncToQueueCount () {
                    if (PlayQueue.count === 0)
                        currentIndex = root.tabs.placeholder;
                    else if (currentIndex === root.tabs.placeholder)
                        currentIndex = PlayQueue.count === 1 && PlayerPresenter.isMediaLoaded ? root.tabs.details : root.tabs.queue;
                }

                Component.onCompleted: syncToQueueCount()

                Connections {
                    target: PlayQueue
                    function onCountChanged () { queueAndMetadata_s.syncToQueueCount() }
                }

                TabPage {
                    name: "placeholder"
                    title: ""   // required by TabPage; this page never gets a button

                    Label {
                        id: queueAndMetadata_p
                        anchors.fill: parent

                        text: qsTr("No media playing right now. Browse or drag one or more audio files here to start playing.")

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment:   Text.AlignVCenter

                        wrapMode: Text.WordWrap

                        // MediumLabel
                        font.pointSize: 13
                        font.weight: Font.Medium

                        property rect globalBounds: Qt.rect(
                                queueAndMetadata_pBounds.x,
                                queueAndMetadata_pBounds.y,
                                queueAndMetadata_p.contentWidth,
                                queueAndMetadata_p.contentHeight
                            )

                        property bool lightMode: Light.lightModeForBounds(globalBounds, root.background, PlayerPresenter)

                        color: lightMode ? Theme.light.texts.primaryText : Theme.dark.texts.primaryText
                    }
                }

                TabPage {
                    name: "queue"
                    title: qsTr("Queue")
                    iconName: "view-media-playlist-symbolic"

                    Playlist {
                        id: queueAndMetadata_c
                        anchors.fill: parent

                        model: PlayQueue

                        scrollBarWidth: rightCol.scrollBarWidth

                        songCoverWidth: root.songCoverWidth
                        songInnerSpacing: root.songInnerSpacing

                        clip: true
                        reuseItems: true // tons of songs moving

                        playheadRowIdx: PlayQueue.playhead.row

                        property rect globalBounds: Qt.rect(
                                queueAndMetadata_cBounds.x + songCoverWidth,
                                queueAndMetadata_cBounds.y,
                                (queueAndMetadata_c.width / 2) - songCoverWidth, // 2nd half is mostly empty space
                                Math.min(queueAndMetadata_c.contentHeight, queueAndMetadata_c.height)
                            )

                        lightMode: Light.lightModeForBounds(globalBounds, root.background, PlayerPresenter)

                        onSongClicked: (song) => {
                            PlayQueue.playhead = PlayQueue.findBySource(song.source)
                        }

                        additionalMenuActions: [

                            T.Action {
                                text: qsTr("Remove from queue")
                                onTriggered: () => {
                                    PlayQueue.remove(queueAndMetadata_c.selectedIndices)
                                    queueAndMetadata_c.selectedIndices = []
                                }
                            },

                            T.Action {
                                text: qsTr("Clear queue")
                                onTriggered: () => PlayQueue.clear()
                            }
                        ]
                    }
                }

                TabPage {
                    name: "details"
                    title: qsTr("Details")
                    iconName: "tag-properties"
                    
                    TrackDetails {
                        id: queueAndMetadata_d

                        leftPadding: metadataContainer.Layout.leftMargin

                        model: root.stateModel.currentTrack

                        lightMode: Light.lightModeForBounds(queueAndMetadata_dBounds.bounds, root.background, PlayerPresenter)

                        spacing: 10
                    }
                }
            }

            SceneRect {
                id: queueAndMetadata_cBounds
                target: queueAndMetadata_c
            }

            SceneRect {
                id: queueAndMetadata_pBounds
                target: queueAndMetadata_p
            }

            SceneRect {
                id: queueAndMetadata_dBounds
                target: queueAndMetadata_d
            }

            SceneRect {
                id: metadataContainerBounds
                target: metadataContainer
            }
        }
    } // rightCol_d
}
