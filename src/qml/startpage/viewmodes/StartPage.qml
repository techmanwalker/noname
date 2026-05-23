import QtQuick

import StartPage
import Primitives
import PlayerModels

Item {
    id: root

    LeftBar {
        id: leftbar

        width: 300
        height: root.height
    }

    Row {
        // Top of the page
        id: topRow

        anchors.left:  leftbar.right
        anchors.right: root.right
        anchors.top:   root.top

        InlineTextField {
            id: searchBar
        }

        anchors.topMargin:    searchBar.font.pixelSize * 3
        anchors.bottomMargin: anchors.topMargin
    }

    Column {
        anchors.left:   leftbar.right
        anchors.right:  root.right
        anchors.top:    topRow.bottom
        anchors.bottom: root.bottom

        // Align the contents to the beginning of the text of the search bar
        leftPadding: searchBar.leftPadding       

        SectionHeading {
            text: "Shortcuts"
        }

        Shortcuts {     
            model: ShortcutsList
        }
    }
}