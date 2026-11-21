pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Player.Primitives
import Player.PlayQueue
import Player.Listings
import Player.LocalLibrary
import Player.MediaTypes

ColumnLayout {
    id: root
    property var directories: LocalLibrary

    // expose to Theater
    property real trackInnerSpacing: 8
    property real trackFadePadding: 10
    property real trackLateralPadding: 15

    // to display the tracklist, title and more
    property directory selectedDirectory

    signal backButtonClicked_r () // from top to down in the hierarchy, marked
    signal trackClicked (track clicked_track)

    onBackButtonClicked_r: {
        fl_stack.currentIndex = 0
    }

    readonly property bool aDirectoryIsOpen: fl_stack.currentIndex === 1

    SectionHeading {
        id: shortcutsHeading

        text: fl_stack.currentIndex == 0 ? qsTr("Folders") : root.selectedDirectory.title;
    }

    StackLayout {
        id: fl_stack
        Layout.fillWidth: true
        Layout.fillHeight: true

        // playlist of playlists
        ListView {
            id: fl

            model: root.directories

            Layout.fillWidth: true
            Layout.fillHeight: true

            MediaSelection {
                id: selection
                model: root.directories

                // a playlist stands for all of its tracks
                sourcesOf: (row) => {
                    const out = []
                    root.directories[row].tracks.forEach(s => out.push(s.source))
                    return out
                }
            }

            delegate: MediaDelegate {
                id: track

                required property directory model
                required property int index

                width: fl.width

                height: 80

                title:      model.title
                cover:      model.cover
                metadata:   qsTr("%n track(s)", "number of tracks in this directory", model.tracks.length) + " · " + model.path
                duration:   model.duration_mmss

                innerSpacing:   root.trackInnerSpacing
                fadePadding:    root.trackFadePadding
                lateralPadding: root.trackLateralPadding

                showSeparator: true

                clip: true

                selected: selection.isSelected(index)

                onClicked: {
                    selection.clear()

                    // lacking the source file still
                    root.selectedDirectory = model

                    // switch the stack to single playlist view
                    fl_stack.currentIndex = 1
                }
                onCtrlClicked: selection.toggle(index)
                onRightClicked: selection.openMenuFor(index)
            }
        }

        MediaGrid {
            id: grid
            model: root.selectedDirectory.tracks

            reuseItems: true

            onTrackClicked: function(clicked_track: track) {
                PlayQueue.switchTo(clicked_track.source)
            }

            additionalMenuActions: [
                T.Action {
                    text: qsTr("Play all")
                    onTriggered: () => PlayQueue.respawnQueue(grid.model)
                }
            ]
        }
    }
}
