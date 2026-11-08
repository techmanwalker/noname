pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T

T.Menu {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    // Qt's own action description: text, enabled, icon, shortcut, triggered().
    // `actions` belong to the menu itself; `extraActions` come from whoever hosts it.
    property list<T.Action> actions
    property list<T.Action> extraActions

    component ActionDelegate: ActionMenuItem {
        required property T.Action modelData
        action: modelData
    }

    contentItem: Column {
        Repeater {
            model: control.actions
            ActionDelegate {}
        }
        Repeater {
            model: control.extraActions
            ActionDelegate {}
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 40
        color: control.palette.window
    }
}
