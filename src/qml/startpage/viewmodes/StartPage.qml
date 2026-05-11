import QtQuick
import QtQuick.Controls

import StartPage
import Primitives

Item {
    id: root

    LeftBar {
        id: leftbar

        width: 300
        height: root.height
    }

    Column {
        anchors.left:   leftbar.right
        anchors.right:  root.right
        anchors.top:    root.top
        anchors.bottom: root.bottom

        SectionHeading {
            text: "Shortcuts"
        }

        Shortcuts {     
            model: ShortcutsList
        }
    }
}