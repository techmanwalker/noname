pragma ComponentBehavior: Bound
import QtQuick

import Player.PlayQueue
import Player.Listings
import Player.MediaTypes

// Selection state, click semantics and context menu for a list/grid of media rows.
// One instance per view; delegates forward their gestures to it.
Item {
    id: root

    // the view's model; `indices` are rows of it
    required property var model

    property list<int> indices: []

    // How a row becomes a song. Defaults to a JS list; item models (the play queue)
    // override it, since `model[row]` doesn't work on them.
    property var songAt: (row) => root.model[row]

    property var sourcesOf: (row) => [root.songAt(row).source]

    property alias extraActions: menu.extraActions

    function isSelected(row: int): bool { return indices.includes(row) }

    function clear(): void { indices = [] }

    function toggle(row: int): void {
        const next = indices.slice() // reassigning guarantees the binding fires
        const at = next.indexOf(row)
        if (at === -1) next.push(row)
        else next.splice(at, 1)
        indices = next
    }

    // a row outside the selection becomes the whole selection. Tells whether it did, for the
    // callers that only want it selected for as long as a gesture lasts (a drag).
    function adopt(row: int): bool {
        if (isSelected(row)) return false
        indices = [row]
        return true
    }

    // the playable sources of `rows`, in their order
    function sourcesFor(rows: list<int>): var {
        const sources = []
        rows.forEach(row => root.sourcesOf(row).forEach(s => sources.push(s)))
        return sources
    }

    // a right-clicked row follows the same rule
    function openMenuFor(row: int): void {
        adopt(row)
        menu.popup()
    }

    SongContextMenu {
        id: menu
        selectedIndices: root.indices

        onClearSelectionRequested: root.clear()
        onAddSelectionToQueue: PlayQueue.batchAppend(root.sourcesFor(root.indices))
    }
}
