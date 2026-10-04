import QtQuick
import QtQuick.Controls

import Player.PlayerPresenter
import Player.Primitives

Column {
    id: root

    property string title
    property string artist
    property string album

    property bool usedInImmersiveMode

    function lightModeForBounds (bounds: rect) : bool {
        return PlayerPresenter.lumaSentinel && (PlayerPresenter.backingLumaForRect(bounds) > 0.55)
                        && ((!usedInImmersiveMode) ?
                            (PlayerPresenter.coverMidringLuma > 0.7)
                          : (PlayerPresenter.coverCentricLuma > 0.7)
                        ) // context is everything
    }

    signal clicked()

    TapHandler {
        onTapped: root.clicked()
    }

    Label {
        id: title

        text: root.title
        font.weight: Font.DemiBold
        font.pointSize: 20

        color: root.lightModeForBounds(Qt.rect(titleBounds.x, titleBounds.y, title.contentWidth, title.contentHeight)) ? "black" : "white"

        visible: root.title.length > 0

        width: parent.width
        elide: Text.ElideRight

        maximumLineCount: 3
        wrapMode: Text.WordWrap
    }

    Label {
        id: artist

        text: root.artist

        visible: root.artist.length > 0

        width: parent.width
        elide: Text.ElideRight

        Binding on color {
            value: "black"
            when: root.lightModeForBounds(Qt.rect(artistBounds.x, artistBounds.y, artist.contentWidth, artist.contentHeight))
        }
    }

    Label {
        id: album

        text: root.album

        visible: root.album.length > 0

        Binding on color {
            value: "black"
            when: root.lightModeForBounds(Qt.rect(albumBounds.x, albumBounds.y, album.contentWidth, album.contentHeight))
        }

        width: parent.width
        elide: Text.ElideRight
    }

    SceneRect {
        id: titleBounds
        target: title

        // onBoundsChanged: console.log("title luma: " + PlayerPresenter.backingLumaForRect(titleBounds))
    }

    SceneRect {
        id: artistBounds
        target: artist

        // onBoundsChanged: console.log("artist luma: " + PlayerPresenter.backingLumaForRect(artistBounds))
    }

    SceneRect {
        id: albumBounds
        target: album

        // onBoundsChanged: console.log("album luma: " + PlayerPresenter.backingLumaForRect(albumBounds))
    }
}