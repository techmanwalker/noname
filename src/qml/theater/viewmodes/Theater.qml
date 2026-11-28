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

    // request
    signal switchToLyricsViewRequested ()
    signal immersiveToggleRequested ()

    // ── Layout ─────────────────────────────────────────────────────────────

    property real trackCoverWidth: 48
    property real trackCoverHeight: trackCoverWidth
    property real trackInnerSpacing: 8
    property real trackFadePadding: 20

    spacing: root.trackCoverWidth

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

            lightMode: Light.lightModeWithPrior(controlsBounds.bounds, root.background, PlayerPresenter, metadataContainer.lightMode)

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
            property int scrollBarWidth: Theme.sizes.scrollBarWidth
            width: (leftCol.width * .6) + (scrollBarWidth * 6)

            spacing: root.trackCoverHeight * .2

            PlayQueueHeader {
                id: metadataContainer
                title:  root.stateModel.title
                artist: root.stateModel.artist
                album:  root.stateModel.album

                Layout.fillWidth: true
                Layout.leftMargin: 15 // playlist track lateral padding
                Layout.rightMargin: Layout.leftMargin

                onClicked: root.immersiveToggleRequested()

                // calculates its lightMode internally
                background: root.background
            }

            Row {
                id: queueAndMetadataTabs

                visible: queueAndMetadata_s.visible && !queueAndMetadata_s.is_showing_no_tracks_placeholder
                Layout.leftMargin: metadataContainer.Layout.leftMargin
                                   - (tabButtons.count > 0 ? tabButtons.itemAt(0).leftPadding : 0)

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

                        lightMode: metadataContainer.lightMode

                        onClicked: queueAndMetadata_s.currentIndex = stackIndex
                    }
                }
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

                // Armed at startup and while the queue is empty; the 0 -> >0 transition is the wait. Any other count change cancels it.
                property bool awaitingMediaLoad: false   // details wanted once the media loads
                property bool autoDetails: false         // currently on details because of an automatic switch
                property bool applyingAutoSwitch: false

                function autoSwitchTo (index) {
                    applyingAutoSwitch = true
                    currentIndex = index
                    applyingAutoSwitch = false
                }

                // Any tab change not made by autoSwitchTo is manual, from any source.
                onCurrentIndexChanged: {
                    if (applyingAutoSwitch)
                        return;
                    awaitingMediaLoad = false
                    autoDetails = false
                }

                function armMediaLoadWait () {
                    awaitingMediaLoad = true
                }

                function syncToQueueCount () {
                    if (PlayQueue.count === 0) {
                        autoDetails = false
                        armMediaLoadWait()
                        autoSwitchTo(root.tabs.placeholder);
                    } else if (currentIndex === root.tabs.placeholder) {
                        autoSwitchTo(root.tabs.queue);  // 0 -> >0 is the wait, keep it armed
                    } else {
                        awaitingMediaLoad = false
                        if (autoDetails && PlayQueue.count > 1) {
                            autoDetails = false
                            autoSwitchTo(root.tabs.queue);
                        }
                    }
                }

                function syncToMediaState () {
                    if (!PlayerPresenter.isMediaLoaded) {
                        // automatic details has nothing to show anymore
                        if (autoDetails) {
                            autoDetails = false
                            autoSwitchTo(root.tabs.queue);
                        }
                        return;
                    }
                    if (!awaitingMediaLoad)
                        return;
                    awaitingMediaLoad = false;
                    if (PlayQueue.count !== 1)
                        return;
                    autoSwitchTo(root.tabs.details);
                    autoDetails = true;
                }

                Component.onCompleted: {
                    armMediaLoadWait()
                    syncToQueueCount()
                }

                Connections {
                    target: PlayQueue
                    function onCountChanged () { queueAndMetadata_s.syncToQueueCount() }
                }

                Connections {
                    target: PlayerPresenter

                    function onTrackChanged () {
                        if (!PlayerPresenter.isMediaLoaded && root.immersive) {
                            root.immersiveToggleRequested()
                        }
                        queueAndMetadata_s.syncToMediaState()
                    }
                }

                component PlaceholderLabel: Label {
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment:   Text.AlignVCenter

                    wrapMode: Text.WordWrap

                    // MediumLabel
                    font.pointSize: 13
                    font.weight: Font.Medium
                }

                TabPage {
                    name: "placeholder"
                    title: ""   // required by TabPage; this page never gets a button

                    PlaceholderLabel {
                        id: queueAndMetadata_p

                        anchors.fill: parent

                        text: qsTr("No media playing right now. Browse or drag one or more audio files here to start playing.")

                        property rect globalBounds: Qt.rect(
                                queueAndMetadata_pBounds.x + (queueAndMetadata_pBounds.width  - contentWidth)  / 2,
                                queueAndMetadata_pBounds.y + (queueAndMetadata_pBounds.height - contentHeight) / 2,
                                contentWidth, contentHeight
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
                        trackAt: (row) => PlayQueue.itemAt(row)

                        scrollBarWidth: rightCol.scrollBarWidth

                        clip: true
                        reuseItems: true // tons of tracks moving

                        playheadRowIdx: PlayQueue.playhead.row

                        // metadata
                        property rect leftGlobalBounds: Qt.rect(
                                queueAndMetadata_cBounds.x + trackCoverWidth,
                                queueAndMetadata_cBounds.y,
                                (queueAndMetadata_c.width / 2) - trackCoverWidth, // 2nd half is mostly empty space
                                Math.min(queueAndMetadata_c.contentHeight, queueAndMetadata_c.height)
                            )

                        property int estimateDurationLabelWidth: (Qt.application.font.pointSize * 4)

                        // duration labels
                        property rect rightGlobalBounds: Qt.rect(
                                queueAndMetadata_cBounds.x + queueAndMetadata_c.width - (trackLateralPadding * 2) - (estimateDurationLabelWidth),
                                queueAndMetadata_cBounds.y,
                                estimateDurationLabelWidth + trackLateralPadding * 2,
                                Math.min(queueAndMetadata_c.contentHeight, queueAndMetadata_c.height)
                            )

                        trackLeftLightMode:  Light.lightModeWithPrior(leftGlobalBounds, root.background, PlayerPresenter,  metadataContainer.lightMode)
                        trackRightLightMode: Light.lightModeWithPrior(rightGlobalBounds, root.background, PlayerPresenter, metadataContainer.lightMode)
                        lightMode: trackLeftLightMode

                        onMoveRequested: (from, to) => PlayQueue.move(from, to)

                        onTrackClicked: (track) => PlayQueue.playhead = PlayQueue.findBySource(track.source)

                        additionalMenuActions: [

                            T.Action {
                                text: qsTr("Save as playlist (UUID name)")
                                onTriggered: () => {
                                    PlayQueue.saveToDisk();
                                }
                            },

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

                    StackLayout {
                        id: queueAndMetadata_d_s

                        anchors.fill: parent

                        currentIndex: PlayerPresenter.isMediaLoaded ? 1 : 0

                        PlaceholderLabel {
                            id: queueAndMetadata_d_p

                            text: qsTr("You have %n queued tracks. (For n=1: Add more tracks from the selector or click Play to play only this track) (For n>1: Press Play to start your queue.).", "Count of tracks in the play queue", PlayQueue.count)

                            property rect globalBounds: Qt.rect(
                                    queueAndMetadata_d_pBounds.x + (queueAndMetadata_d_pBounds.width  - contentWidth)  / 2,
                                    queueAndMetadata_d_pBounds.y + (queueAndMetadata_d_pBounds.height - contentHeight) / 2,
                                    contentWidth, contentHeight
                                )

                            property bool lightMode: Light.lightModeForBounds(globalBounds, root.background, PlayerPresenter)

                            color: lightMode ? Theme.light.texts.primaryText : Theme.dark.texts.primaryText
                        }

                        TrackDetails {
                            id: queueAndMetadata_d_c
                            leftPadding: metadataContainer.Layout.leftMargin

                            model: root.stateModel.currentTrack

                            lightMode: Light.lightModeWithPrior(queueAndMetadata_d_cBounds.bounds, root.background, PlayerPresenter, metadataContainer.lightMode)

                            spacing: 10
                        }
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
                id: queueAndMetadata_d_pBounds
                target: queueAndMetadata_d_p
            }

            SceneRect {
                id: queueAndMetadata_d_cBounds
                target: queueAndMetadata_d_c
            }
        }
    } // rightCol_d
}
