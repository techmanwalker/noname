import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
    id: root
    
    // Model is switchable: NowQueue, PlaylistModel, or any QAbstractListModel
    // The 'model' property is natively handled by ListView
    
    property int songCoverWidth: 48
    property int songCoverHeight: songCoverWidth
    
    // Prevents delegates from being rendered outside the ListView boundaries during scrolling
    clip: true 
    
    spacing: 10

    delegate: Song {
        // Delegate width matches the ListView width
        width: root.width

        // References the delegate's context model
        title: model.title
        cover: model.cover
        artist: model.artist
        duration: model.duration
        coverWidth: root.songCoverWidth
    }

    // Provides visual and functional feedback for scrolling
    ScrollBar.vertical: ScrollBar {
        // Policy and appearance can be customized here
        policy: ScrollBar.AsNeeded
    }
}