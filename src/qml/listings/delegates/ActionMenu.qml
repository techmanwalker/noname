pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Menu {
    id: root

    // Array of { text: string, action: function }. action is called with no
    // arguments on trigger — callers close over whatever context (selection,
    // target item, etc.) their action needs.
    property var actions: []

    Repeater {
        model: root.actions

        MenuItem {
            required property var modelData
            text: modelData.text
            onTriggered: modelData.action()
        }
    }
}