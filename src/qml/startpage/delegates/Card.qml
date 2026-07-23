import QtQuick
import QtQuick.Controls

import Player.Primitives

Column {
    id: root

    property string title
    property string artist

    // only show when mediaType == Card.Song
    property string album
    property int duration: 0 // duration in milliseconds

    readonly property string noTitleText: "Untitled song"
    readonly property string noArtistText: "Unknown artist"

    property int mediaType: MediaTypes.Song // by default

    property url cover
    property real coverWidth: 72
    property real coverHeight: coverWidth

    width: coverWidth // can be no wider than the cover itself plus paddings for aesthetic reasons

    spacing: coverWidth / 8

    signal clicked();

    TapHandler {
        onTapped: root.clicked();
    }

    Cover {
        source: root.cover

        width: root.coverWidth
        height: root.coverHeight
    }

    // Metadata
    Column {
        width: parent.width

        Label {
            id: topLine

            text: root.title.length > 0 ? root.title : root.noTitleText

            width: parent.width


            maximumLineCount: 2
            elide: Text.ElideRight
            wrapMode: Text.WordWrap

            // font properties
            font.pointSize: 14
            font.weight: Font.Medium
            color: "#dfdfdf"
        }

        Label {
            id: bottomLine

            readonly property bool printAlbum: root.mediaType == MediaTypes.Song

            text: (root.artist.length > 0 ? root.artist : root.noArtistText) + (printAlbum ? (" · " + root.album) : "")

            width: parent.width

            maximumLineCount: 2
            elide: Text.ElideRight
            wrapMode: Text.WordWrap
        }

        Label {
            id: durationLabel

            text: Formatters.formatDuration(root.duration)

            visible: root.duration > 0
        }
    }
}