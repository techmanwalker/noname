import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Player

ApplicationWindow {
    id: root
    visible: true
    title: "Noname -"
    color: "#000"

    FlexboxLayout {
        id: contents

        direction: FlexboxLayout.RowReverse

        anchors.fill: parent

        Layout.alignment: Qt.AlignLeft

        LeftSidebar {
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
            model: NextQueue
        }
    }
}