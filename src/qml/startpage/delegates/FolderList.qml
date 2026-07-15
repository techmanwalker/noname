pragma ComponentBehavior: Bound
import QtQuick

import Player.StartPage

ListView {
    id: root

    // expose the songs of the clicked directory
    signal directorySwitched(var songs)

    delegate: Folder {
        id: folder

        required property var model

        name: model.title

        // make the entire delegate react to the clic
        TapHandler {
            onTapped: {
                // emit the signal and expose the songs member
                root.directorySwitched(folder.model.songs)
            }
        }
    }
}