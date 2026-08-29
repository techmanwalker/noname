import QtQuick
import QtQuick.Controls

import Player.Primitives

ResizableButton {
    id: root

    enum ShuffleMode {
        ShuffleTracks,
        ShuffleAlbums,
        Off
    }

    // Use mode name to assign the initial value
    property int currentMode: ShuffleButton.ShuffleMode.Off

    function getShuffleIcon(mode : int) : string {
        switch (mode) {
            case ShuffleButton.ShuffleMode.ShuffleTracks: return "media-playlist-shuffle";
            case ShuffleButton.ShuffleMode.ShuffleAlbums: return "media-random-albums-amarok";
            default:                                     return "media-playlist-no-shuffle";
        }
    }

    icon.name: getShuffleIcon(root.currentMode)

    // Advance to next mode
    onClicked: {
        // Cyclical switching
        const totalModes = 3; // Just hardcode them, QML won't help ya
        root.currentMode = ( (root.currentMode + 1 >= totalModes) ? 0 : root.currentMode + 1);
    }

    Hint {
        visible: root.hovered
        text: {
            switch (root.currentMode) {
                case ShuffleButton.ShuffleMode.ShuffleTracks:    return qsTr("Shuffle tracks")
                case ShuffleButton.ShuffleMode.ShuffleAlbums:    return qsTr("Shuffle albums")
                default:                                         return qsTr("No shuffle")
            }
        }
    }
}
