pragma ComponentBehavior: Bound
import QtQuick

import Player.Listings
import Player.MediaTypes
import Player.Primitives

ListView {
    id: root

    property real shortcutCoverWidth:  Theme.sizes.cardCoverWidth
    property real shortcutCoverHeight: Theme.sizes.cardCoverHeight

    orientation: ListView.Horizontal

    spacing: shortcutCoverWidth / 12

    implicitHeight: contentItem.childrenRect.height

    clip: true

    signal songClicked (song clicked_song)

    // selection
    property alias selectedIndices: selection.indices
    property alias additionalMenuActions: selection.extraActions

    MediaSelection {
        id: selection
        model: root.model
    }

    delegate: MediaDelegate {
        id: shortcut

        card: true

        width: dummycard.width
        height: dummycard.height

        required property song model
        required property int  index

        title: model.title
        metadata: (model.artist.length > 0 ? model.artist : qsTr ("Unknown Artist"))
            + (model.album.length > 0 ? " · " + model.album : "")
        duration: model.duration_mmss

        cover: model.cover

        coverWidth: root.shortcutCoverWidth
        coverHeight: root.shortcutCoverHeight

        // to visually mark it selected
        selected: selection.isSelected(index)

        onClicked: {
            selection.clear() // a plain click drops the multi-selection
            root.songClicked(model)
        }
        onCtrlClicked: selection.toggle(index)
        onRightClicked: selection.openMenuFor(index)
    }

    TallestDummyCard {
        id: dummycard
        visible: false

        coverWidth: root.shortcutCoverWidth
        coverHeight: root.shortcutCoverHeight
    }
}
