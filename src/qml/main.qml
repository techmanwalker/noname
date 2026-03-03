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

    ColumnLayout {
        id: contents
        anchors.centerIn: parent

        PlayingNowCard {
            title: "How Soon Is Now?"
            artist: "t.A.T.u"
            coverFill: "#e6e6b0"
            
            Layout.alignment: Qt.AlignHCenter
        }

        Lyrics {
            Layout.alignment: Qt.AlignHCenter
        }
    }
}