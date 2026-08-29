pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T

T.Menu {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    // Array of { text: string, action: function }. action is called with no
    // arguments on trigger — callers close over whatever context (selection,
    // target item, etc.) their action needs.
    property var actions: []
    

    contentItem: Column {
        Repeater {
            model: control.actions

            ActionMenuItem {
                required property var modelData
                text: modelData.text
                onTriggered: modelData.action()
            }
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 40
        color: control.palette.window
        border.color: control.palette.dark
    }
}