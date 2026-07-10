import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

ColumnLayout {
    id: root

    property string name: "Unnamed folder"
    property var songs

    Label {
        text: root.name
    }

    ListView {
        model: root.songs

        delegate: Song {
            required property var model

            title: model.title
            artist: model.artist
            album: model.album
            cover: model.cover
            duration: model.duration
        }
    }
}