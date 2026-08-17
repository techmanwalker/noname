import QtQuick
import QtQuick.Controls

import Player.Fullscreen

ApplicationWindow {
    id: root

    visible: true
    title: "Noname -"
    color: "#000"

    width: 950
    height: 650

    // rotate between Blurred, Darkened, Both or no filter
    /*
    enum Mode {
        NoFilter = 0,
        Blurred = 1,
        Darkened = 2,
        BlurredDarkened = 3
    }
    */

    property int modeCounter: 0

    Background {
        id: bg
        anchors.fill: parent
    }

    TapHandler {
        onTapped: {
            if (root.modeCounter >= 3) root.modeCounter = 0; else root.modeCounter++;

            bg.blur = root.modeCounter === 1 || root.modeCounter === 3
            bg.darken = root.modeCounter === 2 || root.modeCounter === 3
            
        }
    }


    DropArea {
        anchors.fill: parent

        keys: ["text/uri-list"]

        onDropped: (drop) => {
            if (drop.hasUrls && drop.urls.length > 0) {
                // Get the first URI from the list
                let fileUrl = drop.urls[0].toString();
                
                // Regular expression to check if the extension is a common image format
                let isImage = /\.(jpg|jpeg|png|webp|tiff|bmp)$/i.test(fileUrl);
                
                if (isImage) {
                    bg.source = fileUrl;
                    drop.acceptProposedAction(); // Signal that the drop event was successfully handled
                } else {
                    console.log("Dropped file is not a supported image format:", fileUrl);
                }
            }
        }
    }
}