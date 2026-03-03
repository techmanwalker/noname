import QtQuick
import QtQuick.Controls

Label {
    text: model.text // qmllint disable
    font.pixelSize: 16
    horizontalAlignment: Text.AlignHCenter
    wrapMode: Label.Wrap
    width: parent.width
    
    // Respect font sizing with padding
    topPadding: 8
    bottomPadding: 8
    // implicitHeight is automatically calculated from text height + padding
}