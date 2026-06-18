pragma ComponentBehavior: Bound
import QtQuick

import Player.Primitives
import Player.StartPage


ListView {
    id: root

    property real shortcutCoverWidth: 72
    property real shortcutCoverHeight: shortcutCoverWidth

    orientation: ListView.Horizontal

    spacing: shortcutCoverWidth / 12

    delegate: Card {
        id: shortcut

        required property var model

        title: model.title
        artist: model.artist
        album: model.album ? model.album : ""
        duration: model.duration

        mediaType: MediaTypes.stringMediaTypeToEnum(model.type)

        cover: model.cover

        coverWidth: root.shortcutCoverWidth
        coverHeight: root.shortcutCoverHeight
    }
}