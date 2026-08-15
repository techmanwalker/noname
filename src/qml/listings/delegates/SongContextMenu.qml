pragma ComponentBehavior: Bound
import QtQuick

import Player.PlayerPresenter

ActionMenu {
    id: root

    property var selectedUris: []

    signal clearSelectionRequested ()

    readonly property var defaultActions: [
        {
            text: qsTr("Add to play queue"),
            action: () => {
                if (root.selectedUris.length > 0) {
                    PlayQueue.batch_append(root.selectedUris)
                }
            }
        },
        {
            text: qsTr("Clear selection"),
            action: () => {
                root.clearSelectionRequested ()
            }
        }
    ]

    onClosed: {
        if (root.selectedUris.length === 1) {
            root.clearSelectionRequested ()
        }
    }

    actions: defaultActions
}