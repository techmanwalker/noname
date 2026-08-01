import QtQuick
import QtQuick.Layouts

import Player.Fullscreen
import Player.PlayerPresenter
import Player.Primitives

// Inline view, 
ColumnLayout {
    id: root

    property var stateModel: PlayerPresenter


    readonly property string noTitleText: "Untitled song"
    readonly property string noArtistText: "Unknown artist"
    readonly property string noAlbumText: "Unknown album"

    property real padding: 0
    property real topPadding: padding
    property real bottomPadding: padding
    property real leftPadding: padding
    property real rightPadding: padding

    property real coverToMetadataSpacing: 0

    property bool insetDurationBarInPadding: true

    // you decide if ignore it or not
    signal fullscreenRequested ()

    DurationControl {
        id: duration
        
        Layout.fillWidth: true

        Layout.bottomMargin: root.topPadding - (root.insetDurationBarInPadding ? height : 0)

        stateModel: root.stateModel
    }

    Row {
        id: playerview

        Layout.leftMargin: root.leftPadding
        Layout.rightMargin: root.rightPadding
        Layout.bottomMargin: root.bottomPadding

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

        LabeledButton {
            id: fullscreenToggle
            iconName: "window-maximize"

            text: qsTr("Fullscreen")

            hoverEnabled: true

            leftPadding: 10
            rightPadding: leftPadding

            anchors.verticalCenter: parent.verticalCenter

            onClicked: root.fullscreenRequested()
        }
    }
}