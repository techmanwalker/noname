import QtQuick
import QtQuick.Controls

import Player.Fullscreen
import Player.PlayerPresenter
import Player.Primitives

Column {
    id: root

    property string title
    property string artist
    property string album

    property bool usedInImmersiveMode
    property Background background

    function lightModeForBounds (bounds: rect, name: string) : bool {
        // use the multiplier values background uses
        let multiplier = root.background.lumaMultiplierAtCenter (bounds);

        let m_backdrop = PlayerPresenter.backdropLumaForRect(bounds) * multiplier;
        let m_ring     = PlayerPresenter.ringLumaForRect(bounds)     * multiplier;

        // console.log ("name: " + name + " ; multiplier: " + multiplier + " ; multiplied backdrop luma: " + m_backdrop + " ; ring luma: " + m_ring)

        if (m_backdrop > 0.95) return true;   // always light
        if (m_backdrop < 0.05) return false;  // always dark (redundant while the gate below is 0.48)

        return m_backdrop > 0.48 && m_ring > 0.74;
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

        property rect contentBounds: ({x: titleBounds.x, y: titleBounds.y, width: title.contentWidth, height: title.contentHeight})

        color: (PlayerPresenter.lumaSentinel && root.lightModeForBounds(title.contentBounds, "title")) ? "black" : "white"

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

        property rect contentBounds: ({x: artistBounds.x, y: artistBounds.y, width: artist.contentWidth, height: artist.contentHeight })

        Binding on color {
            value: "black"
            when: PlayerPresenter.lumaSentinel && root.lightModeForBounds(artist.contentBounds, "artist")
        }
    }

    Label {
        id: album

        text: root.album

        visible: root.album.length > 0

        property rect contentBounds: ({x: albumBounds.x, y: albumBounds.y, width: album.contentWidth, height: album.contentHeight})

        Binding on color {
            value: "black"
            when: PlayerPresenter.lumaSentinel && root.lightModeForBounds(album.contentBounds, "album")
        }

        width: parent.width
        elide: Text.ElideRight
    }

    SceneRect {
        id: titleBounds
        target: title

        // onBoundsChanged: console.log("title luma: " + PlayerPresenter.backdropLumaForRect(titleBounds))
    }

    SceneRect {
        id: artistBounds
        target: artist

        // onBoundsChanged: console.log("artist luma: " + PlayerPresenter.backdropLumaForRect(artistBounds))
    }

    SceneRect {
        id: albumBounds
        target: album

        // onBoundsChanged: console.log("album luma: " + PlayerPresenter.backdropLumaForRect(albumBounds))
    }
}