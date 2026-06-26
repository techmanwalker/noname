import QtQuick
import QtQuick.Layouts

import Player
import Player.PlayerPresenter
import Player.Primitives

// Inline view, 
Item {
    id: root

    height: duration.height + playerview.height

    property var stateModel: PlayerPresenter


    readonly property string noTitleText: "Untitled song"
    readonly property string noArtistText: "Unknown artist"
    readonly property string noAlbumText: "Unknown album"

    property real padding: 0
    property real topPadding: padding
    property real bottomPadding: padding
    property real leftPadding: padding
    property real rightPadding: padding

    property real spacing: 0
    property real coverToMetadataSpacing: 0

    property bool insetDurationBarInPadding: false

    DurationControl {
        id: duration
        hideTimestamps: true
        
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: playerview.top

        anchors.bottomMargin: root.topPadding - (root.insetDurationBarInPadding ? height : 0)

        stateModel: root.stateModel
    }

    Row {
        id: playerview

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        anchors.leftMargin: root.leftPadding
        anchors.rightMargin: root.rightPadding
        anchors.bottomMargin: root.bottomPadding

        spacing: root.spacing

        BasicControls {
            anchors.verticalCenter: parent.verticalCenter
        }

        Row {
            spacing: root.coverToMetadataSpacing
            
            Cover {
                width: parent.height
                anchors.verticalCenter: parent.verticalCenter

                source: PlayerPresenter.cover
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter

                readonly property string displayArtist: root.stateModel.artist.length > 0 ? root.stateModel.artist : root.noArtistText
                readonly property string displayAlbum:  root.stateModel.album.length  > 0 ? root.stateModel.album  : root.noAlbumText

                Label {
                    id: title

                    text: root.stateModel.title.length > 0 ? root.stateModel.title : root.noTitleText

                    color: "#dfdfdf"
                }

                Label {
                    id: artistalbum

                    text: parent.displayArtist + " · " + parent.displayAlbum

                    color: "#afafaf"
                }
            }
        }

        VolumeControl {
            anchors.verticalCenter: parent.verticalCenter
            stateModel: root.stateModel

            width: title.font.pointSize * 8
        }
    }
}