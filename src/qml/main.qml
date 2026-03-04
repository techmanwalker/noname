import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

ApplicationWindow {
    id: root
    visible: true
    title: "Noname -"
    color: "#000"

    width: contents.implicitWidth
    height: contents.implicitHeight

    RowLayout {
        id: contents

        anchors.fill: parent

        Layout.alignment: Qt.AlignLeft

        LeftSidebar {
            id: sidebar
            Layout.preferredWidth: font.pointSize * 6
            Layout.preferredHeight: contents.height
        }

        Column {
            PlayingNowCard {
                title: "How Soon Is Now?"
                artist: "t.A.T.u"
                coverFill: "#e6e6b0"
            }

            Lyrics {
                width: parent.width
            }
        }

        Playlist {
            Layout.alignment: Qt.AlignRight
            id: nextQueue
            model: NextQueue
        }
    }
}