pragma ComponentBehavior: Bound
import QtQuick

import Player.Listings
import Player.Primitives

ListView {
    id: root

    property real shortcutCoverWidth: 72
    property real shortcutCoverHeight: shortcutCoverWidth

    orientation: ListView.Horizontal

    spacing: shortcutCoverWidth / 12

    implicitHeight: contentItem.childrenRect.height

    signal songClicked (var song)

    delegate: Card {
        id: shortcut

        required property var model

        title: model.title
        artist: model.artist
        album: model.album ? model.album : ""

        mediaType: MediaTypes.stringMediaTypeToEnum(model.type)

        cover: model.cover

        coverWidth: root.shortcutCoverWidth
        coverHeight: root.shortcutCoverHeight

        onClicked: root.songClicked(model)
    }
}