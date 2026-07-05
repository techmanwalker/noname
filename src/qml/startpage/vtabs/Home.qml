import QtQuick
import QtQuick.Layouts

import Player.MediaSequences
import Player.Primitives
import Player.StartPage

ColumnLayout {
    id: root

    property real shortcutCoverWidth: 144
    property real shortcutCoverHeight: shortcutCoverWidth

    // align with SearchBar text
    property real lateralAlignmentPadding: 0

    SectionHeading {
        id: shortcutsHeading

        text: "Shortcuts"

        Layout.topMargin: shortcutsList.shortcutCoverHeight / 8
        Layout.leftMargin: root.lateralAlignmentPadding
    }

    Shortcuts {
        id: shortcutsList
        model: ShortcutsList

        Layout.fillWidth: true

        Layout.topMargin: shortcutsList.shortcutCoverHeight / 16
        Layout.leftMargin: root.lateralAlignmentPadding

        shortcutCoverWidth: root.shortcutCoverWidth
        shortcutCoverHeight: root.shortcutCoverHeight

        clip: true // prevent sudden disappearing on the edges
    }
}