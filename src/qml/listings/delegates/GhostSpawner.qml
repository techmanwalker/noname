pragma ComponentBehavior: Bound
import QtQuick

import Player.Listings
import Player.MediaTypes
import Player.Primitives

// What a SelectionDrag carries, drawn under the pointer, plus how it leaves: the pick-up,
// the drop onto a DropArea, and the accepted/rejected release animation.
//
// Per view, one GhostSpawner next to the view's SelectionDrag. The view (the "governor")
// owns the gesture and decides how the ghosts look; this owns their lifetime:
//
//     GhostSpawner {
//         selectionDrag: myDrag
//         cards: true                 // false: a ghost playlist
//         dragKey: "noname/tracks"    // what a DropArea listens for
//         lightMode: ...
//     }
//
//   cards: true   N x N cards (columns = ceil(sqrt(n))), enough of them to fit the tracks.
//   cards: false  a single column of rows, like a Playlist with the carried tracks only.
//
// Either way the ghosts are made in the order of the selection (selectionDrag.items), so
// nothing is sorted or reindexed: the first selected track is the first ghost.
Item {
    id: root

    required property SelectionDrag selectionDrag

    property bool cards: true

    // --- The governor picks the theming and sizes of the ghosts.
    property bool lightMode: false
    property real coverWidth:  cards ? Theme.sizes.cardCoverWidth  : Theme.sizes.rowTrackCoverWidth
    property real coverHeight: cards ? Theme.sizes.cardCoverHeight : Theme.sizes.rowTrackCoverHeight

    // cards: the size of the tallest card the grid can make, and the gaps between them
    property real cardWidth:  dummycard.width
    property real cardHeight: dummycard.height
    property real cardColumnSpacing: coverWidth / 12
    property real cardRowSpacing:    coverWidth / 12

    // rows: a width has to be defined, since nothing around the ghosts gives one
    property real rowWidth:  30 * Qt.application.font.pointSize
    property real rowHeight: 80

    // ---

    // Tags the drag, so a DropArea can tell it from other drags (file drops)
    property string dragKey

    // The governor accepts a drop at this moment, so the ghost plays the accepted animation
    property bool acceptsDrop: false

    // The window's root, not the view: the view clips, and the ghosts have to leave it to
    // reach the drop target. Scene coordinates are then this item's own.
    parent: selectionDrag.view.Window.contentItem
    z: 1000

    // snapshot of selectionDrag.items; the ghosts live on while the release plays
    property var carried: []
    property bool releasing: false
    property bool accepted: false

    readonly property real ghostOpacity: 0.4

    visible: selectionDrag.active || releasing
    enabled: false // never takes input
    opacity: ghostOpacity

    width: ghostGrid.width
    height: ghostGrid.height
    x: selectionDrag.pointer.x - width / 2
    y: selectionDrag.pointer.y - height / 2

    // The pointer is the hot spot, so the drop lands where the cursor is
    Drag.active: selectionDrag.active
    Drag.keys: [dragKey]
    Drag.source: selectionDrag
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2

    function begin(): void {
        releaseAnim.stop() // a new press can land while the last release still plays
        releasing = false
        scale = 1
        opacity = ghostOpacity
        carried = selectionDrag.items // set before grabbedRow in arm(), so it is ready
    }

    function play(wasAccepted: bool): void {
        accepted = wasAccepted
        releasing = true
        releaseAnim.restart()
    }

    // The ghosts' lifetime is longer than the gesture's: they are built when armed and
    // reaped once the gesture AND the release animation are over.
    function reap(): void {
        if (selectionDrag.armed || selectionDrag.active || releasing)
            return
        carried = []
        scale = 1
        opacity = ghostOpacity
    }

    Connections {
        target: root.selectionDrag

        function onArmedChanged(): void {
            if (root.selectionDrag.armed) root.begin()
            else root.reap()
        }

        function onActiveChanged(): void {
            if (!root.selectionDrag.active) root.reap() // cancelled mid-drag: no animation
        }

        // Drag.drop() answers with the accepted action, Qt.IgnoreAction when nobody took it
        function onReleased(): void {
            // always deliver the drop: a DropArea may take it even if the governor did not
            const taken = root.Drag.drop() !== Qt.IgnoreAction
            root.play(taken || root.acceptsDrop)
        }
    }

    // One structure for both forms: a column of rows is a grid of one column and no gaps
    Grid {
        id: ghostGrid

        columns: root.cards ? Math.max(1, Math.ceil(Math.sqrt(root.carried.length))) : 1
        columnSpacing: root.cards ? root.cardColumnSpacing : 0
        rowSpacing:    root.cards ? root.cardRowSpacing : 0

        Repeater {
            model: root.carried

            MediaDelegate {
                required property song modelData

                card: root.cards
                lightMode: root.lightMode

                title: modelData.title
                metadata: (modelData.artist.length > 0 ? modelData.artist : qsTr ("Unknown Artist"))
                    + (modelData.album.length > 0 ? " · " + modelData.album : "")
                cover: modelData.cover
                duration: modelData.duration_mmss

                coverWidth: root.coverWidth
                coverHeight: root.coverHeight

                width:  root.cards ? root.cardWidth  : root.rowWidth
                height: root.cards ? root.cardHeight : root.rowHeight

                showSeparator: !root.cards

                selected: true
            }
        }
    }

    // Rejected: a faint red veil over the whole ghost, from the moment of the drop
    Rectangle {
        anchors.fill: parent
        radius: 8
        color: root.lightMode ? Theme.light.elementbg.rejectedDrop
                              : Theme.dark.elementbg.rejectedDrop
        visible: root.releasing && !root.accepted
    }

    // accepted: grows by 35% and fades out. rejected: shrinks to 85% and fades out.
    ParallelAnimation {
        id: releaseAnim

        NumberAnimation {
            target: root
            property: "scale"
            to: root.accepted ? 1.35 : 0.85
            duration: Animations.dragging.release
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: root
            property: "opacity"
            to: 0
            duration: Animations.dragging.release
            easing.type: Easing.OutCubic
        }

        onFinished: {
            root.releasing = false
            root.reap()
        }
    }

    // Only the default of cardWidth/cardHeight: the same card SongGrid sizes its cells with
    TallestDummyCard {
        id: dummycard
        visible: false

        coverWidth: root.coverWidth
        coverHeight: root.coverHeight
    }
}
