pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T

import Player.Listings
import Player.MediaTypes
import Player.PlayQueue

ActionMenu {
    id: root

    required property list<int>  selectedIndices
    required property list<song> sourceModel // what do these indices stand for

    signal addSelectionToQueue ()
    signal clearSelectionRequested ()

    actions: [
        T.Action {
            text: qsTr("Add to play queue")
            onTriggered: root.addSelectionToQueue()
        },
        T.Action {
            text: qsTr("Clear selection")
            onTriggered: root.clearSelectionRequested()
        }
    ]

    onClosed: {
        if (root.selectedIndices.length === 1) {
            root.clearSelectionRequested ()
        }
    }
}
