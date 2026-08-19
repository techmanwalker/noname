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

    signal metadataClicked () // the box containing the cover, title, album and/or artist

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

        Layout.preferredHeight: 48

        spacing: root.spacing

        Layout.fillWidth: true

        BasicControls {
            Layout.alignment: Qt.AlignVCenter
        }

        RowLayout {
            spacing: root.coverToMetadataSpacing

            Layout.preferredWidth: thiscover.width + metadata.width

            TapHandler {
                onTapped: root.metadataClicked()
            }
            
            Cover {
                id: thiscover

                Layout.fillHeight: true
                Layout.preferredWidth: height

                Layout.alignment: Qt.AlignVCenter

                source: PlayerPresenter.cover
            }

            ColumnLayout {
                id: metadata

                Layout.alignment: Qt.AlignVCenter

                Layout.fillHeight: true

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

            Layout.preferredWidth: thiscover.width * 2
        }
    }
}