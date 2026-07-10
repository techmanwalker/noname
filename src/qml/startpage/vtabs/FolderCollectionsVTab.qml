import QtQuick
import QtQuick.Layouts

import Player.Primitives
import Player.MediaSequences

ColumnLayout {
    id: root

    // align with SearchBar text
    property real lateralAlignmentPadding: 0


    SectionHeading {
        id: shortcutsHeading

        text: "Folders"

        Layout.leftMargin: root.lateralAlignmentPadding
    }

    ListView {
        model: FolderCollections
    }
}