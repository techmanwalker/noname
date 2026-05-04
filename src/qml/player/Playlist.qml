import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

ListView {
    id: root

    property int songCoverWidth:  48
    property int songCoverHeight: songCoverWidth
    property int scrollBarWidth: 4

    clip: true

    // Right padding reserves space for the scrollbar so it appears
    // to float outside the list content without overlapping it
    rightMargin: scrollBarWidth * 8

    delegate: Song {
        width: root.width - root.rightMargin

        title:      model.title
        cover:      model.cover
        artist:     model.artist
        duration:   model.duration
        coverWidth: root.songCoverWidth
    }

    ScrollBar.vertical: AccessibleScrollBar {
        barWidth: root.scrollBarWidth
    }
}