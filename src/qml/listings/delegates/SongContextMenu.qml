pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.MediaSequences

Menu {
    id: root

    property var selectedUris: []

    MenuItem {
        text: qsTr("Add to Play Queue")
        onTriggered: {
            if (root.selectedUris.length > 0) {
                PlayQueue.batch_append(root.selectedUris)
            }
        }
    }
}