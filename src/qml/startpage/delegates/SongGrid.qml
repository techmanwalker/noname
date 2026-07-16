pragma ComponentBehavior: Bound
import QtQuick

GridView {
    id: root

    // model: the current folder model with members name, song
    property real songCoverWidth: 72
    property real songCoverHeight: songCoverWidth

    cellWidth: dummycard.width + (songCoverWidth / 12)
    cellHeight: dummycard.height + (songCoverWidth / 12)

    delegate: Card {
        required property var model

        title: model.title
        album: model.album
        artist: model.artist
        duration: model.duration
        cover: model.cover

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight
    }

    LargestDummyCard {
        id: dummycard
        visible: false

        coverWidth: root.songCoverWidth
        coverHeight: root.songCoverHeight
    }
}