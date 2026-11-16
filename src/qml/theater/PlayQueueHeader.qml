import QtQuick
import QtQuick.Controls

import Player.Theater
import Player.Presenter
import Player.Primitives

Column {
    id: root

    property string title
    property string artist
    property string album

    // no need to be very precise, used to calculate the backdrop lightness
    readonly property int textContentWidth: Math.max(title_it.contentWidth, artist_it.contentWidth, album_it.contentWidth)
    readonly property int textContentHeight: title_it.contentHeight + artist_it.contentHeight + album_it.contentHeight + spacing * 2

    readonly property alias titleBounds: titleBounds_.bounds

    required property Background background

    readonly property alias lightMode: title_it.lightMode

    signal clicked()

    TapHandler {
        onTapped: root.clicked()
    }

    Label {
        id: title_it

        text: root.title
        font.weight: Font.DemiBold
        font.pointSize: 20

        // the label spans the column; the glyphs fill contentWidth × contentHeight from its top-left
        readonly property rect inkBounds: Qt.rect(titleBounds_.x, titleBounds_.y, contentWidth, contentHeight)

        property bool lightMode: Light.lightModeForBounds(inkBounds, root.background, PlayerPresenter)

        color: title_it.lightMode ? Theme.light.texts.primaryText : Theme.dark.texts.primaryText

        visible: root.title.length > 0

        width: parent.width
        elide: Text.ElideRight

        maximumLineCount: 3
        wrapMode: Text.WordWrap
    }

    SceneRect {
        id: titleBounds_
        target: title_it
    }

    Label {
        id: artist_it

        text: root.artist.length > 0 ? root.artist : qsTr("Unknown Artist")

        visible: root.title.length > 0

        width: parent.width
        elide: Text.ElideRight

        color: title_it.lightMode ? Theme.light.texts.secondaryText : Theme.dark.texts.secondaryText
    }

    Label {
        id: album_it

        text: root.album

        visible: root.title.length > 0 && root.album.length > 0

        color: title_it.lightMode ? Theme.light.texts.secondaryText : Theme.dark.texts.secondaryText

        width: parent.width
        elide: Text.ElideRight
    }
}
