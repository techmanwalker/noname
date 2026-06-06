pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Player
import Primitives

ListView {
    id: root

    property int itemCoverWidth: 144
    property int itemCoverHeight: itemCoverWidth

    // DUMMY delegate just to calculate the theoretical max height
    Song {
        id: dummy
        visible: false
        coverWidth: root.itemCoverWidth
        card: true

        hideDuration: true
    }

    orientation: ListView.Horizontal
    spacing: 5
    height: dummy.cardMaxHeight + scrollBar.logicalWidth
    width: parent ? parent.width : 300

    clip: true

    delegate: Song {
        required property var model
        
        title:      model.title
        cover:      model.cover
        artist:     model.artist
        album:      model.album
        duration:   model.duration
        coverWidth: root.itemCoverWidth
        card:       true

        hideDuration: true
    }

    ScrollBar.horizontal: AccessibleScrollBar {
        id: scrollBar

        visible: false

        logicalWidth: scrollBar.barWidth
    }
}