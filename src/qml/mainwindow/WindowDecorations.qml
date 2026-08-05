import QtQuick
import QtQuick.Controls

import Player.Primitives

// Window decorations

Row {
    id: root
    layoutDirection: Qt.RightToLeft

    required property Window window

    ToolButton {
        icon.name: "window-close"
        onClicked: root.window.close()
    }

    ToolButton {
        icon.name: "window-maximize"
        onClicked: {
                if (root.window.visibility === Window.Maximized) {
                    root.window.showNormal()
                } else {
                    root.window.showMaximized()
                }
            }
    }

    ToolButton {
        icon.name: "window-minimize"
        onClicked: root.window.showMinimized()
    }    
}