import QtQuick

import Player.Primitives

// Window decorations

Row {
    id: root
    layoutDirection: Qt.RightToLeft

    required property Window window

    readonly property real  closeButtonRightPadding: close.rightPadding
    readonly property alias squareButtonWidth: close.width

    ResizableButton {
        id: close
        icon.name: "window-close"
        onClicked: root.window.close()
    }

    ResizableButton {
        icon.name: "window-maximize"
        onClicked: {
                if (root.window.visibility === Window.Maximized) {
                    root.window.showNormal()
                } else {
                    root.window.showMaximized()
                }
            }
    }

    ResizableButton {
        icon.name: "window-minimize"
        onClicked: root.window.showMinimized()
    }    
}