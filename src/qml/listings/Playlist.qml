pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.MediaTypes
import Player.Primitives
import Player.PlayQueue

ListView {
    id: root

    property bool lightMode: false

    property int playheadRowIdx

    // Gap above the playing row, as a fraction of that row's own height
    property real playheadMarginRatio: 0.4

    property int autoscrollCooldownMs: 15000

    property int songCoverWidth:  48
    property int songCoverHeight: songCoverWidth
    property int scrollBarWidth: 4

    // expose to Theater
    property real songInnerSpacing: 8
    property real songFadePadding: 10
    property real songLateralPadding: 15

    // Right padding reserves space for the scrollbar so it appears
    // to float outside the list content without overlapping it
    rightMargin: scrollbar.logicalWidth

    signal songClicked(song clicked_song)

    // selection

    property list<url> selectedSources: []
    property alias additionalMenuActions: songContextMenu.extraActions

    // true while the user drives the view (flick, wheel, drag or scrollbar)
    readonly property bool userInteracting: moving || scrollbar.pressed

    // autoscroll is suspended while the user interacts, and for a cooldown after
    readonly property bool autoscrollAllowed: !userInteracting && !cooldownTimer.running

    function scrollToPlayhead() : void {
        if (playheadRowIdx < 0 || playheadRowIdx >= count)
            return

        const fromY = contentY

        // Qt resolves the destination, clamped to the content bounds. This also
        // instantiates the target delegate, so it can be queried right after.
        positionViewAtIndex(playheadRowIdx, ListView.Beginning)

        let toY = contentY

        // Within the first/last page the row can't reach the margin anyway, so
        // Qt's clamped position is already the right one. Anywhere else, lift
        // the row by its own height fraction.
        const item = itemAtIndex(playheadRowIdx)
        if (item && !atYBeginning && !atYEnd)
            toY -= playheadMarginRatio * item.height

        contentY = fromY

        scrollAnimation.to = toY
        scrollAnimation.restart()
    }

    NumberAnimation {
        id: scrollAnimation
        target: root
        property: "contentY"
        duration: 125
        easing.type: Easing.InOutCubic
    }

    // Fires the deferred scroll once the user has been idle long enough, so a
    // playhead change that happened mid-interaction is not lost.
    Timer {
        id: cooldownTimer
        interval: root.autoscrollCooldownMs
        onTriggered: root.scrollToPlayhead()
    }

    onUserInteractingChanged: {
        if (userInteracting) {
            // yield to the user immediately, and make sure the timer can't
            // fire a scroll in the middle of their gesture
            scrollAnimation.stop()
            cooldownTimer.stop()
        } else {
            cooldownTimer.restart()
        }
    }

    onPlayheadRowIdxChanged: if (autoscrollAllowed) scrollToPlayhead()

    Component.onCompleted: Qt.callLater(scrollToPlayhead)

    SongContextMenu {
        id: songContextMenu
        selectedUris: root.selectedSources

        onClearSelectionRequested: root.selectedSources = []
    }

    delegate: Song {
        id: song

        required property song modelData
        required property int index
        
        title:      modelData.title
        cover:      modelData.cover
        metadata:   (modelData.artist.length > 0 ? modelData.artist : qsTr ("Unknown Artist"))
            + (modelData.album.length > 0 ? " · " + modelData.album : "")
        duration:   modelData.duration_mmss

        width: root.width - root.rightMargin

        height: 80

        lightMode: root.lightMode

        coverWidth: root.songCoverWidth

        innerSpacing:   root.songInnerSpacing
        fadePadding:    root.songFadePadding
        lateralPadding: root.songLateralPadding

        // findBySource returns a different object from memory, hence comparing .row instead
        playing: playheadRowIdx >= 0 // qmllint disable
            && index === playheadRowIdx // qmllint disable

        showSeparator: true

        clip: true

        selected: root.selectedSources.includes(modelData.source)

        onClicked: {
            // Standard click clears multi-selection
            root.selectedSources = []
            root.songClicked(modelData)
        }

        onCtrlClicked: {
            let sources = root.selectedSources
            let idx = sources.indexOf(modelData.source)
            
            if (idx === -1) {
                sources.push(modelData.source)
            } else {
                sources.splice(idx, 1)
            }
            
            // Reassign with slice() to guarantee the QML property binding is triggered
            root.selectedSources = sources.slice() 
        }

        onRightClicked: {
            // If right-clicked song isn't in current selection, select only it
            if (!root.selectedSources.includes(modelData.source)) {
                root.selectedSources = [modelData.source]
            }
            songContextMenu.popup()
        }
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 3

        policy: ScrollBar.AsNeeded

        lightMode: root.lightMode
    }
}
