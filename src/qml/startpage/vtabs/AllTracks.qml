import QtQuick
import QtQuick.Layouts

import Player.Primitives

ColumnLayout {
    id: root

    // align with SearchBar text
    property real lateralAlignmentPadding: 0


    SectionHeading {
        id: shortcutsHeading

        text: "All tracks"

        Layout.leftMargin: root.lateralAlignmentPadding
    }
}