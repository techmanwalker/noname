import QtQuick
import QtQuick.Layouts

import Player.Theater
import Player.Presenter
import Player.Primitives
import Player.PlayQueue

// Inline view.
// An Item, not a layout: the DropArea and its marker have to cover the whole bar, and an
// Item declared inside a layout is laid out as one more row instead.
Item {
    id: root

    property PlayerPresenter stateModel: PlayerPresenter

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    readonly property string noTitleText: qsTr("Untitled track")
    readonly property string noArtistText: qsTr("Unknown Artist")

    // was the ColumnLayout's own: still the spacing of the rows below
    property real spacing: 5

    property real padding: 0
    property real topPadding: padding
    property real bottomPadding: padding
    property real leftPadding: padding
    property real rightPadding: padding

    property real volumeControlBottomPadding: 0 // visual compensation

    property real coverWidth: 48
    property real coverHeight: coverWidth

    property real coverToMetadataSpacing: 0

    property bool insetDurationBarInPadding: true

    property bool showDropHereBanner: false

    // Drags carrying one of these keys can be dropped here. Empty: nothing is accepted
    // (a DropArea without keys would take any drag).
    property var dropKeys: []

    // From an accepted drop until its tracks are on the queue. batchAppend is asynchronous,
    // so an owner that hides this bar while the queue is empty should keep it up meanwhile.
    // Set by this item; owners only read it.
    property bool isAcceptingADrop: false

    signal metadataClicked () // the box containing the cover, title, album and/or artist

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: root.spacing

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

                    source: root.stateModel.isMediaLoaded ? root.stateModel.cover
                        : (PlayQueue.count > 0 ? PlayQueue.itemAt(0).cover : "")

                    sourceSize: Qt.size(root.coverWidth, root.coverHeight)
                }

                ColumnLayout {
                    id: metadata

                    Layout.alignment: Qt.AlignVCenter

                    Layout.fillHeight: true

                    readonly property string displayArtist: root.stateModel.artist.length > 0 ? root.stateModel.artist : root.noArtistText

                    Label {
                        id: title

                        text: {
                            if (root.stateModel.isMediaLoaded) {
                                return root.stateModel.title.length > 0 ? root.stateModel.title : root.noTitleText
                            } else {
                                if (PlayQueue.count === 0) {
                                    return qsTr("No tracks in queue.");
                                } else {
                                    return qsTr("%n track(s) in queue.", "", PlayQueue.count)
                                }
                            }
                        }

                        color: Theme.dark.texts.secondaryText

                        elide: Text.ElideRight

                        Layout.fillWidth: true
                    }

                    Label {
                        id: artistalbum

                        text: {
                            if (root.showDropHereBanner) {
                                return qsTr("Drop here to add to the queue…")
                            } else {
                                if (root.stateModel.isMediaLoaded) {
                                    return metadata.displayArtist + ((root.stateModel.album.length > 0) ? (" · " + root.stateModel.album) : "")
                                } else {
                                    return qsTr("Next: %1").arg(PlayQueue.count > 0 ? PlayQueue.itemAt(0).title : "")
                                }
                            }
                        }

                        color: Theme.dark.texts.tertiaryText

                        elide: Text.ElideRight

                        Layout.fillWidth: true
                    }
                }
            }

            Item {
                Layout.alignment: Qt.AlignVCenter

                Layout.preferredWidth: thiscover.width * 8

                Layout.preferredHeight: parent.height

                VolumeControl {
                    stateModel: root.stateModel

                    anchors.fill: parent
                    anchors.bottomMargin: root.volumeControlBottomPadding

                    spacing: muteButtonWidth / 2

                    visible: root.stateModel.isMediaLoaded
                }
            }
        }
    }

    // Takes the drop of tracks. Takes no mouse input: clicks reach the controls underneath.
    DropArea {
        id: dropArea

        anchors.fill: parent
        keys: root.dropKeys
        enabled: root.dropKeys.length > 0

        onDropped: (drop) => {
            // whatever carries the drag exposes the playable sources it holds
            const sources = drop.source?.sources ?? []
            if (sources.length === 0) {
                drop.accepted = false
                return
            }

            root.isAcceptingADrop = PlayQueue.count === 0
            if (root.isAcceptingADrop)
                giveUp.restart()

            PlayQueue.batchAppend(sources)
            drop.acceptProposedAction()
        }
    }

    // nothing valid was appended (unreadable files, say): don't keep the bar up forever
    Timer {
        id: giveUp

        interval: 5000
        onTriggered: root.isAcceptingADrop = false
    }

    Connections {
        target: PlayQueue

        function onCountChanged(): void {
            if (PlayQueue.count > 0) {
                root.isAcceptingADrop = false
                giveUp.stop()
            }
        }
    }

    /*
    // Drop marker: shown while a drag of tracks hovers the bar
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: root.topPadding // don't paint the padding
        z: 10

        radius: 8
        color: Qt.rgba(1, 1, 1, 0.12) // swap for your Theme colors
        border.width: 2
        border.color: Qt.rgba(1, 1, 1, 0.6)

        opacity: dropArea.containsDrag ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
    }
    */
}
