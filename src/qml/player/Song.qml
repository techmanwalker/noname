import QtQuick
import QtQuick.Controls

import Player.Primitives
import Player.MediaSequences

Item {
    id: root

    readonly property string noTitleText: "Untitled song"
    readonly property string noArtistText: "Unknown artist"
    readonly property string noAlbumText: "Unknown album"
    readonly property string noDurationText: "-:--"
    property bool playing: false

    property string title: noTitleText
    property string artist: noArtistText
    property string album: noAlbumText
    property url cover
    property int duration: 0  // Duration in milliseconds
    property int coverWidth: 48
    property int coverHeight: coverWidth

    // card form: suitable for the start page
    property bool card: false
    property int maxFirstLineLines: 3
    property int maxSecondLineLines: 2

    // for Playlists and such; give the programmer ability to hide data
    property bool hideAlbum: false
    property bool hideDuration: false

    // Helper function to format milliseconds as mm:ss
    function formatDuration(ms) {
        var totalSeconds = Math.floor(ms / 1000)
        var m = Math.floor(totalSeconds / 60)
        var s = totalSeconds % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

    signal clicked()

    TapHandler {
        onTapped: root.clicked();
    }

    height: root.card ? cardMaxHeight : coverHeight * 1.4

    width: root.card ? coverWidth : undefined

    clip: true

    Cover {
        id: coverItem

        width: root.coverWidth
        height: root.coverHeight
        source: root.cover

        anchors.verticalCenter: root.card ? undefined : parent.verticalCenter
    }

    Column {
        id: metadata

        anchors.verticalCenter: root.card ? undefined : parent.verticalCenter
        anchors.left: root.card ? undefined : coverItem.right
        anchors.right: root.card ? undefined : durationContainer.left
        anchors.top: root.card ? coverItem.bottom : undefined
        
        anchors.leftMargin: root.card ? undefined : 10
        anchors.topMargin: root.card ? 15 : undefined

        width: root.card ? root.width : undefined
        height: firstLine.height + secondLine.height

        Label {
            id: firstLine

            text: root.title.length > 0 ? root.title : root.noTitleText

            // eliding
            maximumLineCount: root.maxFirstLineLines
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            width: parent.width

            // font properties
            font.pointSize: root.card ? 14 : -1
            font.weight: root.card ? Font.DemiBold : Font.Normal
        }

        Label {
            id: secondLine

            property string displayArtist: root.artist.length > 0 ? root.artist : root.noArtistText
            property string displayAlbum:  root.album.length  > 0 ? root.album  : root.noAlbumText

            text: root.hideAlbum
                ? displayArtist
                : displayArtist + " · " + displayAlbum

            // eliding
            maximumLineCount: root.maxSecondLineLines
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            width: parent.width
        }
    }

    Column {
        id: durationContainer
        
        anchors.top: root.card ? metadata.bottom : undefined
        anchors.right: root.card ? undefined : parent.right
        anchors.verticalCenter: root.card ? undefined : parent.verticalCenter

        visible: root.duration > 0 && !root.hideDuration

        Label {
            id: durationLabel
            text: root.duration > 0 ? root.formatDuration(root.duration) : root.noDurationText
        }
    }




    // theoretical max height calculators — invisible, never rendered
    Text {
        id: firstLineMaxCalc
        visible: false
        width: root.coverWidth
        text: Array(root.maxFirstLineLines + 1).join("W\n")
        font: firstLine.font
        wrapMode: Text.WordWrap
        maximumLineCount: root.maxFirstLineLines
    }

    Text {
        id: secondLineMaxCalc
        visible: false
        width: root.coverWidth
        text: Array(root.maxSecondLineLines + 1).join("W\n")
        font: secondLine.font
        wrapMode: Text.WordWrap
        maximumLineCount: root.maxSecondLineLines
    }

    Text {
        id: durationLineMaxCalc
        visible: false
        width: root.coverWidth
        text: "-:--"
        font: durationLabel.font
    }

    readonly property real cardMaxHeight: root.coverHeight
        + firstLineMaxCalc.contentHeight
        + secondLineMaxCalc.contentHeight
        + (
            !root.hideDuration ? durationLineMaxCalc.contentHeight : 0
        )
}