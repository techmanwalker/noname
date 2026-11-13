import QtQuick

import Player.Primitives

// The press-and-hold drag gesture of a view's rows, carrying the view's selection. It owns
// the gesture (arming, the Flickable lock, the pointer, the Esc cancel) and nothing about
// what a release means: that is answered by whoever listens to released() (SingleRowMove
// moves a lone row, SongGrid hands the tracks to a DropArea through a Drag).
//
// Per view, one SelectionDrag next to the view's MediaSelection. Per delegate, the two
// handlers below, declared as direct children of the delegate root (as TapHandler and
// DragHandler were in Playlist, so the delegate's own click handling is not disturbed):
//
//     SelectionDrag.Arm   { controller: myDrag; row: delegate.index }
//     SelectionDrag.Track { controller: myDrag; row: delegate.index }
//
// (No `pragma ComponentBehavior: Bound` here, on purpose: Qt refuses to instantiate a bound
// file's inline components, Arm and Track, from other files.)
//
// The gesture has two stages, like it always had in Playlist:
//   armed   a long press that stayed put. The view stops being a Flickable for the rest of
//           the press, so a sweep never arms and an armed press can't be stolen.
//   active  the pointer then actually moved: this is the drag.
// (An Item only so that it can hold children; it has no geometry and draws nothing itself.)
Item {
    id: root

    required property Flickable view
    required property MediaSelection selection

    // Most rows a drag carries
    property real maxSelectedItems: Infinity

    // Keys that cancel an armed/active drag. Empty: the drag can only end by releasing.
    // A Shortcut outranks every focused item, so an owner that sets this must also disable
    // any other shortcut on the same keys while armed is true (ambiguous overload).
    property var cancelSequences: []

    // The row the long press landed on: >= 0 from arming until the gesture ends
    property int grabbedRow: -1
    readonly property bool armed: grabbedRow >= 0

    // The pointer is really dragging (armed AND moved)
    property bool active: false

    // What this gesture carries, in selection order. Snapshots taken when the long press
    // arms, valid until the next one: they stay readable while released() is delivered and
    // the model is free to change under a drag.
    property list<int> rows
    property var items: []      // the model items of `rows`
    property var sources: []    // the playable sources of `rows`

    // Pointer in scene coordinates, and where in the grabbed delegate it was pressed
    property point pointer
    property point grabOffset

    // The gesture ended with the pointer released mid-drag. Emitted with armed already
    // false and active still true.
    signal released()

    // Whether `row` is part of what is being carried (for dimming the origin)
    function carries(row: int): bool {
        return armed && rows.includes(row)
    }

    // true when this gesture is what selected the grabbed row: that selection is the
    // gesture's and goes away with it
    property bool adopted: false

    function arm(row: int): void {
        // a row outside the selection (or no selection at all) becomes the only selected
        // row, exactly like right-clicking it
        adopted = selection.adopt(row)

        // The drag carries a COPY of the head of the selection, cut to maxSelectedItems.
        // The selection itself is deliberately left untouched: the cut belongs to this
        // gesture and must not change what stays highlighted or what the context menu
        // would act on.
        rows = selection.indices.slice(0, maxSelectedItems)
        items = rows.map(r => selection.songAt(r))
        sources = selection.sourcesFor(selection.indices) // only the ghosts are capped

        // last: this is what locks the view and dims the rows
        grabbedRow = row
    }

    function begin(scene: point, press: point): void {
        if (!armed)
            return
        track(scene, press)
        active = true
    }

    function track(scene: point, press: point): void {
        if (!armed)
            return // cancelled while the pointer is still down: the rest of the gesture is ignored
        pointer = scene
        grabOffset = press
    }

    // Ends the gesture, whichever handler notices the release first. Idempotent. State is
    // reset BEFORE emitting: whatever the release causes (a move of the playing row, say)
    // must find the view already free, so that re-enabling autoscroll restarts its cooldown
    // and the change is deferred instead of yanking the view.
    function end(): void {
        if (!armed)
            return
        const dragged = active
        grabbedRow = -1
        if (dragged)
            released()
        settle()
    }

    function cancel(): void {
        if (!armed)
            return
        grabbedRow = -1
        settle()
    }

    function settle(): void {
        active = false
        if (adopted)
            selection.clear()
        adopted = false
    }

    // An armed press belongs to the drag: the Flickable must not steal it
    Binding {
        target: root.view
        property: "interactive"
        value: false
        when: root.armed
    }

    Shortcut {
        sequences: root.cancelSequences
        enabled: root.armed
        onActivated: root.cancel()
    }

    // Arms the drag. Moving past the drag threshold before the timer fires cancels this
    // handler, so a scroll sweep never arms.
    component Arm: TapHandler {
        required property SelectionDrag controller
        required property int row

        acceptedButtons: Qt.LeftButton
        acceptedModifiers: Qt.NoModifier
        longPressThreshold: Animations.dragging.threshold / 1000 // seconds

        onLongPressed: controller.arm(row)

        // Ends an armed press that never became a drag. When it does become one, Track is
        // already active before this handler gives up at the system drag distance, so this
        // must leave the gesture alone.
        onPressedChanged: if (!pressed && !controller.active) controller.end()
    }

    // Follows the pointer once armed.
    component Track: DragHandler {
        required property SelectionDrag controller
        required property int row

        target: null // the delegate stays where the view put it; only the gesture is tracked

        // Unarmed, no axis is draggable, so a sweep can't start this handler and stays the
        // Flickable's. Armed, the first movement starts the drag.
        xAxis.enabled: controller.grabbedRow === row
        yAxis.enabled: controller.grabbedRow === row
        dragThreshold: 0

        onActiveChanged: {
            if (active)
                controller.begin(centroid.scenePosition, centroid.pressPosition)
            else
                controller.end()
        }

        onCentroidChanged: if (active) controller.track(centroid.scenePosition, centroid.pressPosition)
    }
}
