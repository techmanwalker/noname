import QtQuick

import Player.StartPage

ListView {
    id: root

    delegate: Folder {
        id: folder

        required property var model

        name: model.name
        songs: model.songs
    }
}