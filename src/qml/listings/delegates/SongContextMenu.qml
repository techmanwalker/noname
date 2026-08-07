pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.Primitives
import Player.MediaSequences

ActionMenu {
    id: root

    property var selectedUris: []

    readonly property var defaultActions: [
        {
            text: qsTr("Add to play queue"),
            action: () => {
                if (root.selectedUris.length > 0) {
                    PlayQueue.batch_append(root.selectedUris)
                }
            }
        }
    ]

    actions: defaultActions
}