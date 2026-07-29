import QtQuick
import QtQuick.Controls

Label {
    property var lyric: ({})

    property int timestamp: lyric.timestamp || -1 // no timestamp set

    text: lyric.text || ""
    font.pointSize: 10
    horizontalAlignment: Text.AlignHCenter
    wrapMode: Label.Wrap
    width: implicitWidth
    
    // Respect font sizing with padding
    topPadding: 8
    bottomPadding: 8
    // implicitHeight is automatically calculated from text height + padding
}