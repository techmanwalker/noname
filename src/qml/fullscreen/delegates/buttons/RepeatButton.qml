import QtQuick
import QtQuick.Controls

import Player.Primitives

ResizableButton {
    id: root

    // Define statuses
    enum RepeatMode {
        Off,
        RepeatSingle,
        RepeatAllTracks
    }

    // Initial state
    property int currentMode: RepeatButton.RepeatMode.Off

    // Map state -> icon
    function getRepeatIcon(mode : int) : string {
        switch (mode) {
            case RepeatButton.RepeatMode.RepeatSingle:    return "media-playlist-repeat-song";
            case RepeatButton.RepeatMode.RepeatAllTracks: return "media-playlist-repeat";
            case RepeatButton.RepeatMode.Off:             return "media-repeat-none";
            default:                                      return "media-repeat-none";
        }
    }

    // Assign icon dynamically
    icon.name: getRepeatIcon(root.currentMode)

    // Cyclical rotation logic
    onClicked: {
        // (0+1)%3 = 1 -> (1+1)%3 = 2 -> (2+1)%3 = 0 ... and so on
        root.currentMode = (root.currentMode + 1) % 3;
    }
    
    // Hovered tooltip
    Hint {
        visible: root.hovered
        text: {
            switch (root.currentMode) {
                case RepeatButton.RepeatMode.RepeatSingle:    return qsTr("Repeat one")
                case RepeatButton.RepeatMode.RepeatAllTracks: return qsTr("Repeat all")
                default:                                      return qsTr("Repeat disabled")
            }
        }
    }
}