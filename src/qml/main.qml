import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

ApplicationWindow {
    id: root
    visible: true
    title: "Noname -"
    color: "#000"

    Row {
        id: contents

        anchors.fill: parent

        Layout.alignment: Qt.AlignLeft

        LeftSidebar {
            width: font.pointSize * 6
            height: contents.height
        }

        ColumnLayout {
            PlayingNowCard {
                title: "How Soon Is Now?"
                artist: "t.A.T.u"
                coverFill: "#e6e6b0"
            }

            Lyrics {
            }
        }
    }
}