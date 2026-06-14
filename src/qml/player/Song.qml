import QtQuick
import QtQuick.Controls

import Player
import Player.Primitives

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

    // visual separation
    property bool showSeparator: false

    // more geometry control
    property real leftPadding:   0
    property real rightPadding:  0
    property real topPadding:    0
    property real bottomPadding: 0
    property real innerSpacing:  0
    property real fadePadding:   0 // symmetric fading distance from the borders inwards

    // Helper function to format milliseconds as mm:ss
    function formatDuration(ms) {
        var totalSeconds = Math.floor(ms / 1000)
        var m = Math.floor(totalSeconds / 60)
        var s = totalSeconds % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

    signal clicked()

    height: metadata.height + topPadding + bottomPadding

    clip: true

    // factored out due to code extension
    SongBackground {
        anchors.fill: parent

        playing: root.playing
        hovered: hover.hovered

        outer_leftstop:   0
        inner_leftstop:   root.fadePadding               / Math.max(1, root.width)
        inner_rightstop: (root.width - root.fadePadding) / Math.max(1, root.width)
        outer_rightstop:  1
    }

    // Separator bottom border
    Rectangle {
        visible: root.showSeparator
        color: Qt.rgba(160, 160, 160, .1)

        width: root.width
        height: 1

        anchors.bottom: parent.bottom
    }

    HoverHandler {
        id: hover
    }

    TapHandler {
        onTapped: root.clicked();
    }

    Item {
        id: metadata

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.leftPadding
        anchors.rightMargin: root.rightPadding

        height: Math.max(coverItem.height, metadataLines.height, durationLabel.height)

        y: root.topPadding
        x: root.leftPadding

        Cover {
            id: coverItem

            width: root.coverWidth
            height: root.coverHeight
            source: root.cover

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
        }

        Item {
            id: metadataLines

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: coverItem.right
            anchors.right: durationLabel.left

            anchors.leftMargin: root.innerSpacing
            anchors.rightMargin: root.innerSpacing

            height: firstLine.height + secondLine.height

            Label {
                id: firstLine

                text: root.title.length > 0 ? root.title : root.noTitleText

                width: parent.width

                // eliding
                maximumLineCount: root.maxFirstLineLines
                wrapMode: Text.WordWrap
                elide: Text.ElideRight

                // font properties
                font.pointSize: -1
                font.weight: Font.Normal
            }

            Label {
                id: secondLine

                anchors.top: firstLine.bottom

                width: parent.width

                property string displayArtist: root.artist.length > 0 ? root.artist : root.noArtistText
                property string displayAlbum:  root.album.length  > 0 ? root.album  : root.noAlbumText

                text: root.hideAlbum
                    ? displayArtist
                    : displayArtist + " · " + displayAlbum

                // eliding
                maximumLineCount: root.maxSecondLineLines
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
            }
        }

        Label {
            id: durationLabel

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            visible: root.duration > 0 && !root.hideDuration

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