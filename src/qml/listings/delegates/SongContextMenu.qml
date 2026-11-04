pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T

import Player.Listings
import Player.PlayQueue

ActionMenu {
    id: root

    property list<url> selectedUris

    signal clearSelectionRequested ()

    actions: [
        T.Action {
            text: qsTr("Add to play queue")
            onTriggered: PlayQueue.batchAppend(root.selectedUris)
        },
        T.Action {
            text: qsTr("Clear selection")
            onTriggered: root.clearSelectionRequested()
        }
    ]

    onClosed: {
        if (root.selectedUris.length === 1) {
            root.clearSelectionRequested ()
        }
    }
}