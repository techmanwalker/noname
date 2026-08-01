pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Listings
import Player.Primitives
import Player.MediaSequences

ListView {
    id: root
    
    signal songClicked(var song)

    property int songCoverWidth:  48
    property int songCoverHeight: songCoverWidth
    property int scrollBarWidth: 4

    // expose to FullscreenPlayer
    property real songInnerSpacing: 8
    property real songFadePadding: 10

    // Right padding reserves space for the scrollbar so it appears
    // to float outside the list content without overlapping it
    rightMargin: scrollbar.logicalWidth

    delegate: Song {
        id: song

        required property var model
        
        width: root.width - root.rightMargin

        height: 80

        title:      model.title
        cover:      model.cover
        artist:     model.artist
        album:      model.album
        duration:   model.duration

        coverWidth: root.songCoverWidth

        innerSpacing:  root.songInnerSpacing
        fadePadding:   root.songFadePadding
        lateralPadding: 15

        playing: PlayQueue.playhead === PlayQueue.index(model.index, 0)
        onClicked: root.songClicked(model)

        showSeparator: true
    }

    ScrollBar.vertical: AccessibleScrollBar {
        id: scrollbar
        barWidth: root.scrollBarWidth
        logicalWidth: root.scrollBarWidth * 3
    }
}