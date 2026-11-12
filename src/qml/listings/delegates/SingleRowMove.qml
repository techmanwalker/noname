pragma ComponentBehavior: Bound

import QtQuick

// What a SelectionDrag means in a ListView whose rows can be reordered: carrying exactly
// one row and releasing it over another row moves it. Carrying any other number of rows is
// not a reorder, so nothing is tracked, drawn or reported for it.
//
// This only reports the gesture; committing the move is the consumer's job.
// (An Item only so that it can hold children; it has no geometry and draws nothing itself.)
Item {
    id: root

    required property ListView view
    required property SelectionDrag selectionDrag

    // Colors the drop marker
    property bool lightMode: false

    // Where the row would be put. from is the carried row, to the row it was released over.
    signal moveRequested(int from, int to)

    // A single row is being dragged
    readonly property bool moving: selectionDrag.active && selectionDrag.rows.length === 1

    // The row under the pointer while moving, -1 when none
    readonly property int overRow: {
        if (!moving)
            return -1

        const p = view.mapFromItem(null, selectionDrag.pointer.x, selectionDrag.pointer.y)
        const y = view.contentY + p.y
        const row = view.indexAt(view.contentX, y)

        // below the last row is a legal drop: it means "to the end"
        return row >= 0 ? row : (y > view.contentHeight ? view.count - 1 : -1)
    }

    Connections {
        target: root.selectionDrag

        function onReleased(): void {
            const from = root.selectionDrag.rows[0]
            if (root.moving && root.overRow >= 0 && root.overRow !== from)
                root.moveRequested(from, root.overRow)
        }
    }

    // Drop marker. Moving down lands below the target row, moving up lands above it,
    // matching move_item's "ends up at dest's row" semantics.
    Rectangle {
        parent: root.view // overlay on the viewport, not inside the scrolling content
        z: 10

        readonly property Item target: root.overRow >= 0 ? root.view.itemAtIndex(root.overRow) : null
        readonly property int from: root.selectionDrag.rows[0] ?? -1

        visible: target !== null && root.overRow !== from
        width: root.view.width - root.view.rightMargin
        height: 2
        y: target
            ? target.y - root.view.contentY + (from < root.overRow ? target.height : 0) - height / 2
            : 0

        color: root.lightMode ? "black" : "white" // swap for your Theme color
    }
}
