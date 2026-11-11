import QtQuick

import Player.Primitives

// Window decorations

Row {
    id: root
    layoutDirection: Qt.RightToLeft

    required property Window window

    readonly property real  closeButtonRightPadding: close.rightPadding
    readonly property alias squareButtonWidth: close.width

    property bool lightMode: false

    signal toggleMaximize_b () // top to bottom or vice versa, bidirectional

    onToggleMaximize_b: {
        if (root.window.visibility === Window.Maximized) {
            root.window.showNormal()
        } else {
            root.window.showMaximized()
        }
    }

    ResizableButton {
        id: close
        icon.name: "window-close"
        onClicked: root.window.close()

        lightMode: root.lightMode
    }

    ResizableButton {
        icon.name: "window-maximize"
        onClicked: root.toggleMaximize_b()

        lightMode: root.lightMode
    }

    ResizableButton {
        icon.name: "window-minimize"
        onClicked: root.window.showMinimized()

        lightMode: root.lightMode
    }    
}
