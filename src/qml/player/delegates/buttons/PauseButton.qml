import QtQuick
import QtQuick.Controls

ToolButton {
    id: root

    property bool paused: true
    
    icon.name: (root.paused ? "media-playback-start" : "media-playback-pause")
}