pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Primitives
import Player.MediaSequences

ListView {
    id: root

    property int songCoverWidth:  48
    property int songCoverHeight: songCoverWidth
    property int scrollBarWidth: 4

    // expose to FullscreenPlayer

    property real songTopPadding: 12
    property real songBottomPadding: 12
    property real songLeftPadding: 24
    property real songRightPadding: 24
    property real songInnerSpacing: 8
    property real songFadePadding: 20

    // Right padding reserves space for the scrollbar so it appears
    // to float outside the list content without overlapping it
    rightMargin: scrollbar.logicalWidth

    delegate: Song {
        id: song
        required property var model
        
        width: root.width - root.rightMargin

        title:      model.title
        cover:      model.cover
        artist:     model.artist
        album:      model.album
        duration:   model.duration
        coverWidth: root.songCoverWidth

        maxSecondLineLines: 1

        topPadding:    root.songTopPadding
        bottomPadding: root.songBottomPadding
        leftPadding:   root.songLeftPadding
        rightPadding:  root.songRightPadding
        innerSpacing:  root.songInnerSpacing
        fadePadding:   root.songFadePadding

        playing: PlayQueue.playhead === PlayQueue.index(model.index, 0)
        onClicked: PlayQueue.playhead = PlayQueue.index(model.index, 0)

        showSeparator: true
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 3
    }
}