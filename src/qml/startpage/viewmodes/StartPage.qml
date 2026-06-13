import QtQuick

import StartPage
import Player
import Player.Primitives
import Player.MediaSequences

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

    Item {
        anchors.left:   leftbar.right
        anchors.right:  root.right
        anchors.top:    topRow.bottom
        anchors.bottom: root.bottom

        // Align the contents to the beginning of the text of the search bar
        anchors.leftMargin: searchBar.leftPadding       

        SectionHeading {
            anchors.top: parent.top
            anchors.left: parent.left
            
            id: shortcutsHeading
            text: "Shortcuts"
        }

        /*
        Rectangle {
            anchors.fill: shortcutsList

            color: "#00f"
        }*/


        Playlist {
            id: shortcutsList
            model: PlayQueue

            // avoid content overflow
            anchors.top:    shortcutsHeading.bottom
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.bottom: parent.bottom
        }
    }
}