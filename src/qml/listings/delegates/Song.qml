import QtQuick
import QtQuick.Layouts

import Player.Primitives

Item {
    id: root

    property bool card: false

    property string title: "Untitled song"
    property string artist: "Unknown artist"
    property string album

    property int duration: 0 // in milliseconds

    property url cover: ""

    property int coverWidth: root.card ? 144 : 48
    property int coverHeight: coverWidth

    property int innerSpacing: coverWidth / 8

    property bool hideAlbum
    property bool hideDuration
    property bool showSeparator

    property bool playing
    property int lateralPadding
    property int fadePadding

    property int maxFirstLineLines: root.card ? 2 : 1
    property int maxSecondLineLines: root.card ? 2 : 1

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    signal clicked ();

    SongBackground {
        anchors.fill: parent

        playing: root.playing
        hovered: hover.hovered

        outer_leftstop:   0
        inner_leftstop:   root.fadePadding               / Math.max(1, root.width)
        inner_rightstop: (root.width - root.fadePadding) / Math.max(1, root.width)
        outer_rightstop:  1
    }

    HoverHandler {
        id: hover
    }

    TapHandler {
        onTapped: root.clicked();
    }

    FlexboxLayout {
        id: content
        anchors.fill: parent

        anchors.leftMargin: root.lateralPadding
        anchors.rightMargin: root.lateralPadding

        rowGap: root.innerSpacing
        columnGap: root.innerSpacing

        direction: root.card ? FlexboxLayout.Column : FlexboxLayout.Row

        alignItems: root.card ? FlexboxLayout.AlignStart : FlexboxLayout.AlignCenter

        Cover {
            source: root.cover

            Layout.preferredWidth:  root.coverWidth
            Layout.preferredHeight: root.coverHeight
        }

        Column {
            Layout.fillWidth: true 

            Label {
                id: firstLine

                text: root.title

                width: parent.width

                maximumLineCount: root.maxFirstLineLines
                elide: Text.ElideRight
                wrapMode: Text.WordWrap

                font.pointSize: root.card ? 14 : -1
                font.weight: Font.Medium
                color: "#dfdfdf"
            }

            Label {
                id: secondLine

                text: root.artist + ((root.hideAlbum || root.album.length == 0) ? "" : " · " + root.album)

                width: parent.width

                maximumLineCount: root.maxSecondLineLines
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
            }
        }

        Label {
            id: durationLabel

            text: Formatters.formatDuration(root.duration)

            visible: !root.hideDuration
        }
    }
}