import QtQuick
import QtQuick.Layouts

import Player.Fullscreen
import Player.PlayerPresenter
import Player.Primitives

// Inline view, 
ColumnLayout {
    id: root

    property PlayerPresenter stateModel: PlayerPresenter


    readonly property string noTitleText: qsTr("Untitled track")
    readonly property string noArtistText: qsTr("Unknown artist")

    property real padding: 0
    property real topPadding: padding
    property real bottomPadding: padding
    property real leftPadding: padding
    property real rightPadding: padding

    property real coverWidth: 48
    property real coverHeight: coverWidth

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

            Layout.fillWidth: true

            TapHandler {
                onTapped: root.metadataClicked()
            }
            
            Cover {
                id: thiscover

                Layout.preferredWidth: root.coverWidth
                Layout.preferredHeight: root.coverHeight

                Layout.alignment: Qt.AlignVCenter

                source: PlayerPresenter.cover

                sourceSize: Qt.size(root.coverWidth, root.coverHeight)
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

                    text: metadata.displayArtist + ((root.stateModel.album.length > 0) ? (" · " + root.stateModel.album) : "")

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