import QtQuick
import QtQuick.Layouts

Column {
    id: root
    
    // Make model switchable - can be NowQueue, PlaylistModel, or any QAbstractListModel
    property var model
    property int songCoverWidth: 48
    property int songCoverHeight: songCoverWidth
    
    Repeater {
        id: repeater
        model: root.model
        
        delegate: Song {
            width: root.width

            // Use just 'model' (the delegate's implicit model), not 'root.model'

            title: model.title
            cover: model.cover
            artist: model.artist
            duration: model.duration
            coverWidth: root.songCoverWidth
        }
    }
}