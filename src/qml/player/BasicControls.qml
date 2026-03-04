import QtQuick
import QtQuick.Controls

Row {
    id: root
    property bool paused: true

    ToolButton {
        icon.name: "media-skip-backward"
    }

    ToolButton {
        icon.name: (root.paused ? "media-playback-start" : "media-playback-pause")
    }

    ToolButton {
        icon.name: "media-skip-forward"
    }
}