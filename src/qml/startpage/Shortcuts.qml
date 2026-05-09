import QtQuick
import QtQuick.Controls
import Player
import Primitives

ListView {
    id: root

    property int itemCoverWidth:  96
    property int itemCoverHeight: itemCoverWidth
    
    orientation: ListView.Horizontal
    spacing: 20
    clip: true

    // keep updated with Song's card height
    height: itemCoverHeight * 1.4
    
    // try to fit its parent
    width: parent ? parent.width : 300 

    delegate: Song {
        title:      model.title
        cover:      model.cover
        artist:     model.artist
        duration:   model.duration
        coverWidth: root.itemCoverWidth
        
        card: true
        width: root.itemCoverWidth // horizontal scroll
    }

    ScrollBar.horizontal: AccessibleScrollBar {}
}