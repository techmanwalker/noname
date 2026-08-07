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

    property real padding: 0
    property real topPadding: padding
    property real bottomPadding: padding
    property real leftPadding: padding
    property real rightPadding: padding

    property real coverToMetadataSpacing: 0

    property bool insetDurationBarInPadding: true

    DurationControl {
        id: duration
        
        Layout.fillWidth: true

        Layout.bottomMargin: root.topPadding - (root.insetDurationBarInPadding ? height : 0)

        stateModel: root.stateModel
    }

    RowLayout {
        id: playerview

        Layout.leftMargin: root.leftPadding
        Layout.rightMargin: root.rightPadding
        Layout.bottomMargin: root.bottomPadding

        spacing: root.spacing

        Layout.fillWidth: true

        BasicControls {
            Layout.alignment: Qt.AlignVCenter
        }

        RowLayout {
            spacing: root.coverToMetadataSpacing

            Layout.fillWidth: true
            
            Cover {
                Layout.preferredWidth: parent.height
                Layout.preferredHeight: parent.height

                Layout.alignment: Qt.AlignVCenter

                source: PlayerPresenter.cover
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true

                readonly property string displayArtist: root.stateModel.artist.length > 0 ? root.stateModel.artist : root.noArtistText

                Label {
                    id: title

                    text: root.stateModel.title.length > 0 ? root.stateModel.title : root.noTitleText

                    color: "#dfdfdf"

                    elide: Text.ElideRight

                    Layout.fillWidth: true
                }

                Label {
                    id: artistalbum

                    text: parent.displayArtist + ((root.stateModel.album.length > 0) ? (" · " + root.stateModel.album) : "")

                    color: "#afafaf"

                    elide: Text.ElideRight

                    Layout.fillWidth: true
                }
            }
        }

        VolumeControl {
            Layout.alignment: Qt.AlignVCenter
            stateModel: root.stateModel

            Layout.preferredWidth: title.font.pointSize * 12
        }
    }
}