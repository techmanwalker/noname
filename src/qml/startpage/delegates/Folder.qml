import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

ColumnLayout {
    id: root

    property string name: "Unnamed folder"

    Label {
        text: root.name
    }
}