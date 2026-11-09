pragma ComponentBehavior: Bound
import QtQuick

import Player.PlayQueue
import Player.Listings

// Selection state, click semantics and context menu for a list/grid of media rows.
// One instance per view; delegates forward their gestures to it.
Item {
    id: root

    // the view's model; `indices` are rows of it
    required property var model

    property list<int> indices: []

    // playable sources a row stands for: a song is itself, a playlist would be its tracks
    property var sourcesOf: (row) => [root.model[row].source]

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

    // a right-clicked row outside the selection becomes the whole selection
    function openMenuFor(row: int): void {
        if (!isSelected(row)) indices = [row]
        menu.popup()
    }

    SongContextMenu {
        id: menu
        selectedIndices: root.indices

        onClearSelectionRequested: root.clear()
        onAddSelectionToQueue: {
            const sources = []
            root.indices.forEach(row => root.sourcesOf(row).forEach(s => sources.push(s)))
            PlayQueue.batchAppend(sources)
        }
    }
}
