import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root
    
    // Make model switchable - can be NowQueue, PlaylistModel, or any QAbstractListModel
    property var model
    property int songCoverWidth: 48
    property int songCoverHeight: songCoverWidth

    Layout.preferredWidth: implicitWidth
    Layout.preferredHeight: implicitHeight
    
    Repeater {
        id: repeater
        model: root.model
        
        delegate: Song {
            Layout.fillWidth: true
            // Use just 'model' (the delegate's implicit model), not 'root.model'

            title: model.title
            cover: model.cover
            artist: model.artist
            duration: model.duration
            coverWidth: root.songCoverWidth
        }
    }
}